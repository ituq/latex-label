#include <QWidget>
#include <QString>
#include <QPainter>
#include <QRect>
#include <QGuiApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <iostream>
#include "CodeBlockWidget.h"

#include <md4c.h>
#include <vector>
#include "latex.h"
#include "core/formula.h"
#include "render.h"
#include "element.h"


struct parsedString {
    QString text;
    int type; // Changed from LabelType to int for now
};
enum class fragment_type{
    latex,
    text,
    line,
    rounded_rect
};
enum class font_type{
    normal,
    bold,
    italic,
    italic_bold,
    mono,
    strikethrough,
    underline,
    hyperlink,
    heading1,
    heading2,
    heading3,
    heading4,
    heading5,
    heading6
};

struct frag_text_data{
    QString text;
    QFont font;
};
struct frag_line_data{
    QPoint to;
    int width;
};
struct frag_rrect_data{
    QRect rect;
    qreal topLeftRadius;
    qreal topRightRadius;
    qreal bottomLeftRadius;
    qreal bottomRightRadius;
    QPalette::ColorRole background;

    // Constructor with individual corner radii
    frag_rrect_data(QRect r, qreal tl, qreal tr, qreal bl, qreal br, QPalette::ColorRole bg)
        : rect(r), topLeftRadius(tl), topRightRadius(tr), bottomLeftRadius(bl), bottomRightRadius(br), background(bg) {}

    // Constructor with uniform radius for all corners
    frag_rrect_data(QRect r, qreal radius, QPalette::ColorRole bg)
        : rect(r), topLeftRadius(radius), topRightRadius(radius), bottomLeftRadius(radius), bottomRightRadius(radius), background(bg) {}
};
struct frag_latex_data{
    tex::TeXRender* render;
    QString text;
    bool isInline;
};
typedef struct Fragment{
    QRect bounding_box;
    fragment_type type;
    bool is_highlighted;
    void* data;
    Fragment(QRect bounding_box,QString text, QFont font):bounding_box(bounding_box),is_highlighted(false),type(fragment_type::text){
        //text constructor
        data = new frag_text_data(text,font);
    }
    Fragment(QRect bounding_box, QRect rect,int radius,QPalette::ColorRole bg):bounding_box(bounding_box),is_highlighted(false),type(fragment_type::rounded_rect){
        //rounded rect constructor
        data = new frag_rrect_data(rect,radius,bg);
    }
    Fragment(QRect bounding_box, QRect rect, qreal tl, qreal tr, qreal bl, qreal br, QPalette::ColorRole bg):bounding_box(bounding_box),is_highlighted(false),type(fragment_type::rounded_rect){
        //rounded rect constructor with individual corner radii
        data = new frag_rrect_data(rect,tl,tr,bl,br,bg);
    }
    Fragment(QRect bounding_box, QPoint to, int width = 1): bounding_box(bounding_box),is_highlighted(false), type(fragment_type::line){
        //line constructor
        data = new frag_line_data(to,width);
    }
    Fragment(QRect bounding_box, tex::TeXRender* render, QString text, bool isInline): bounding_box(bounding_box),is_highlighted(false), type(fragment_type::latex){
        data = new frag_latex_data(render,text,isInline);
    }

    //Stream operator for easy printing with QDebug
    friend QDebug operator<<(QDebug debug, const Fragment& fragment) {
        QString result = QString("Fragment{");
        result += QString("type: ");

        switch(fragment.type) {
            case fragment_type::text:
                result += "text";
                break;
            case fragment_type::latex:
                result += "latex";
                break;
            case fragment_type::line:
                result += "line";
                break;
            case fragment_type::rounded_rect:
                result += "rounded_rect";
                break;
        }

        result += QString(", bounding_box: (%1,%2,%3,%4)")
                    .arg(fragment.bounding_box.x())
                    .arg(fragment.bounding_box.y())
                    .arg(fragment.bounding_box.width())
                    .arg(fragment.bounding_box.height());

        result += QString(", highlighted: %1").arg(fragment.is_highlighted ? "true" : "false");

        //Add type-specific data
        switch(fragment.type) {
            case fragment_type::text: {
                frag_text_data* text_data = (frag_text_data*)fragment.data;
                QString text_content = text_data->text;
                if(text_content.length() > 50) {
                    text_content = text_content.left(47) + "...";
                }
                text_content = text_content.replace('\n', "\\n").replace('\t', "\\t");
                result += QString(", text: \"%1\"").arg(text_content);
                result += QString(", font: %1 %2pt").arg(text_data->font.family()).arg(text_data->font.pointSize());
                break;
            }
            case fragment_type::latex: {
                frag_latex_data* latex_data = (frag_latex_data*)fragment.data;
                QString latex_text = latex_data->text;
                if(latex_text.length() > 50) {
                    latex_text = latex_text.left(47) + "...";
                }
                result += QString(", latex: \"%1\"").arg(latex_text);
                result += QString(", inline: %1").arg(latex_data->isInline ? "true" : "false");
                if(latex_data->render) {
                    result += QString(", render_size: %1x%2")
                                .arg(latex_data->render->getWidth())
                                .arg(latex_data->render->getHeight());
                }
                break;
            }
            case fragment_type::line: {
                frag_line_data* line_data = (frag_line_data*)fragment.data;
                result += QString(", to: (%1,%2)").arg(line_data->to.x()).arg(line_data->to.y());
                result += QString(", width: %1").arg(line_data->width);
                break;
            }
            case fragment_type::rounded_rect: {
                frag_rrect_data* rect_data = (frag_rrect_data*)fragment.data;
                result += QString(", rect: (%1,%2,%3,%4)")
                            .arg(rect_data->rect.x())
                            .arg(rect_data->rect.y())
                            .arg(rect_data->rect.width())
                            .arg(rect_data->rect.height());
                result += QString(", radii: (tl:%1,tr:%2,bl:%3,br:%4)")
                            .arg(rect_data->topLeftRadius)
                            .arg(rect_data->topRightRadius)
                            .arg(rect_data->bottomLeftRadius)
                            .arg(rect_data->bottomRightRadius);
                break;
            }
        }

        result += "}";
        debug.noquote() << result;
        return debug;
    }
}Fragment;

//Stream operator for standard C++ streams
inline std::ostream& operator<<(std::ostream& os, const Fragment& fragment) {
    QString result = QString("Fragment{");
    result += QString("type: ");

    switch(fragment.type) {
        case fragment_type::text:
            result += "text";
            break;
        case fragment_type::latex:
            result += "latex";
            break;
        case fragment_type::line:
            result += "line";
            break;
        case fragment_type::rounded_rect:
            result += "rounded_rect";
            break;
    }

    result += QString(", bounding_box: (%1,%2,%3,%4)")
                .arg(fragment.bounding_box.x())
                .arg(fragment.bounding_box.y())
                .arg(fragment.bounding_box.width())
                .arg(fragment.bounding_box.height());

    result += QString(", highlighted: %1").arg(fragment.is_highlighted ? "true" : "false");

    //Add type-specific data
    switch(fragment.type) {
        case fragment_type::text: {
            frag_text_data* text_data = (frag_text_data*)fragment.data;
            QString text_content = text_data->text;
            if(text_content.length() > 50) {
                text_content = text_content.left(47) + "...";
            }
            text_content = text_content.replace('\n', "\\n").replace('\t', "\\t");
            result += QString(", text: \"%1\"").arg(text_content);
            result += QString(", font: %1 %2pt").arg(text_data->font.family()).arg(text_data->font.pointSize());
            break;
        }
        case fragment_type::latex: {
            frag_latex_data* latex_data = (frag_latex_data*)fragment.data;
            QString latex_text = latex_data->text;
            if(latex_text.length() > 50) {
                latex_text = latex_text.left(47) + "...";
            }
            result += QString(", latex: \"%1\"").arg(latex_text);
            result += QString(", inline: %1").arg(latex_data->isInline ? "true" : "false");
            if(latex_data->render) {
                result += QString(", render_size: %1x%2")
                            .arg(latex_data->render->getWidth())
                            .arg(latex_data->render->getHeight());
            }
            break;
        }
        case fragment_type::line: {
            frag_line_data* line_data = (frag_line_data*)fragment.data;
            result += QString(", to: (%1,%2)").arg(line_data->to.x()).arg(line_data->to.y());
            result += QString(", width: %1").arg(line_data->width);
            break;
        }
        case fragment_type::rounded_rect: {
            frag_rrect_data* rect_data = (frag_rrect_data*)fragment.data;
            result += QString(", rect: (%1,%2,%3,%4)")
                        .arg(rect_data->rect.x())
                        .arg(rect_data->rect.y())
                        .arg(rect_data->rect.width())
                        .arg(rect_data->rect.height());
            result += QString(", radii: (tl:%1,tr:%2,bl:%3,br:%4)")
                        .arg(rect_data->topLeftRadius)
                        .arg(rect_data->topRightRadius)
                        .arg(rect_data->bottomLeftRadius)
                        .arg(rect_data->bottomRightRadius);
            break;
        }
    }

    result += "}";
    os << result.toStdString();
    return os;
}

// Parser state for md4c callbacks
struct MarkdownParserState {
    std::vector<Element*> segments;
    std::vector<Element*> blockStack;
    std::vector<Element*> spanStack;
    QString currentText;
    int textSize;
    int list_nesting_level;
    std::vector<Element> list_type_stack; //track nested list types


    MarkdownParserState(int size) : textSize(size), list_nesting_level(0) {}
};
tex::TeXRender* getLatexRenderer(const QString& latex, bool isInline, int text_size, QRgb argb_color);


class LatexLabel : public QWidget{

public:
    void appendText(MD_TEXTTYPE type, QString& text);
    void appendText(QString& text); // Legacy overload for backward compatibility
    void appendBlock(MD_BLOCKTYPE type, std::string data);
    void appendSpan(MD_SPANTYPE type, std::string data);
    void setText(QString text);
    void setTextSize(int size);
    int getTextSize() const;
    QSize sizeHint() const override;
    LatexLabel(QWidget* parent=nullptr);
    ~LatexLabel();

    //Debug method to print m_segments structure
    void printSegmentsStructure() const;


private:
    QPalette m_pallete = QGuiApplication::palette();
    std::vector<Fragment> m_display_list;
    std::vector<parsedString> content;
    tex::TeXRender* _render;
    QString m_text;
    QString m_raw_text; //without markdown formatting
    std::vector<Element*> m_segments;
    int m_textSize;
    double m_leading=3.0;
    bool m_is_dragging=false;
    Fragment* m_selected=nullptr;

    int margin_left=5, margin_right=5,margin_top=5,margin_bottom=5;

    //Widgets for rendering code blocks
    std::vector<CodeBlockWidget*> m_code_widgets;

    //void parseText();
    void parseMarkdown(const QString& text);

    // Markdown rendering helpers
    void renderBlock(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderSpan(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal lineHeight, QFont* font_passed=nullptr);
    void renderListElement(const Element& segment, qreal& x, qreal& y, int min_x,int max_x, qreal& lineHeight);
    void renderHeading(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderCodeBlock(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderBlockquote(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderTextSegment(const Element* segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal lineHeight);
    void renderTable(const Element& segment, qreal& x, qreal& y, qreal min_x, qreal max_x, qreal& lineHeight);
    void calculate_table_dimensions(const Element& segment, int* max_width_of_col, int* max_height_of_row, int columns, int rows, qreal min_x, qreal max_x);

    // Font and styling helpers
    QFont getFont(const Element* segment) const;
    QFont getFont(font_type type) const;
    qreal getLineHeight(const Element& segment, const QFontMetricsF& metrics) const;

    // md4c callback functions
    static int enterBlockCallback(MD_BLOCKTYPE type, void* detail, void* userdata);
    static int leaveBlockCallback(MD_BLOCKTYPE type, void* detail, void* userdata);
    static int enterSpanCallback(MD_SPANTYPE type, void* detail, void* userdata);
    static int leaveSpanCallback(MD_SPANTYPE type, void* detail, void* userdata);
    static int textCallback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata);

    // Simple incomplete LaTeX detection
    bool hasIncompleteLatexAtEnd(const QString& text);

    // Debug helper method
    void printSegmentRecursive(const Element* element, int depth) const;

    // Cleanup methods for AST elements
    void cleanup_segments(std::vector<Element*>& elements);  // free all pointers in AST
    void deleteDisplayList();

    //Helpers for managing embedded code block widgets
    void clear_code_block_widgets();
    void create_code_block_widget(const QRect& rect, const QString& text, const QFont& font, const QString& language = QString());

    // Fragment creation helper methods for better readability
    void addText(qreal x, qreal y, qreal width, qreal height, const QString& text, const QFont& font);
    void addLatex(qreal x, qreal y, qreal width, qreal height, tex::TeXRender* render, const QString& text, bool isInline);
    void addLine(qreal x, qreal y, qreal width, qreal height, const QPoint& to, int lineWidth = 1);
    void addRoundedRect(qreal x, qreal y, qreal width, qreal height, const QRect& rect, qreal radius, QPalette::ColorRole bg);
    void addRoundedRect(qreal x, qreal y, qreal width, qreal height, const QRect& rect, qreal tl, qreal tr, qreal bl, qreal br, QPalette::ColorRole bg);

protected:
    void paintEvent(QPaintEvent* event) override;
    void changeEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};
