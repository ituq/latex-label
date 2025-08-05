#include <QWidget>
#include <QString>
#include <QPainter>
#include <QRect>
#include <QGuiApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QFocusEvent>

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
    link,
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
    qreal radius;
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
    Fragment(QRect bounding_box, QRect rect,int radius):bounding_box(bounding_box),is_highlighted(false),type(fragment_type::rounded_rect){
        //rounded rect constructor
        data = new frag_rrect_data(rect,radius);
    }
    Fragment(QRect bounding_box, QPoint to, int width = 1): bounding_box(bounding_box),is_highlighted(false), type(fragment_type::line){
        //line constructor
        data = new frag_line_data(to,width);
    }
    Fragment(QRect bounding_box, tex::TeXRender* render, QString text, bool isInline): bounding_box(bounding_box),is_highlighted(false), type(fragment_type::latex){
        data = new frag_latex_data(render,text,isInline);
    }
}Fragment;

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
tex::TeXRender* getLatexRenderer(const QString& latex, bool isInline);


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

    //void parseText();
    void parseMarkdown(const QString& text);

    // Markdown rendering helpers
    void renderBlock(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderSpan(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal lineHeight, QFont* font_passed=nullptr);
    void renderListElement(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
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

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};
