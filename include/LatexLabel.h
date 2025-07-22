#include <QWidget>
#include <QString>
#include <QPainter>
#include <QRect>

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
/*






struct MarkdownAttributes {
    int headingLevel = 0;
    QString url;
    QString title;
    QString alt;
    int listStartNumber = 1;
    bool isTight = false;
    char listMarker = '*';
    QString codeLanguage;
    MarkdownBlockType parentListType = MarkdownBlockType::UnorderedList; //track parent list type for list items
    int listItemNumber = 1; //track item number for ordered lists

    MarkdownAttributes() = default;
};
*/



//
// SUPPORTED SPAN TYPES
//


/*
struct TextSegment {
    QString content;
    TextSegmentType type;
    tex::TeXRender* render;

    // Markdown-specific fields
    MarkdownBlockType blockType;
    MarkdownSpanType spanType;
    MarkdownTextType textType;
    MarkdownAttributes attributes;

    // For nested content and styling
    QFont font;
    QColor color;
    Qt::Alignment alignment;
    int indentLevel = 0;
    bool isNested = false;
    std::vector<TextSegment> children;

    TextSegment() : render(nullptr), blockType(MarkdownBlockType::Document),
                   spanType(MarkdownSpanType::Emphasis), textType(MarkdownTextType::Normal),
                   color(Qt::black), alignment(Qt::AlignLeft) {}
};

*/
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
    std::vector<parsedString> content;
    tex::TeXRender* _render;
    QString m_text;
    std::vector<Element*> m_segments;
    int m_textSize;
    double m_leading=3.0;

    //void parseText();
    void parseMarkdown(const QString& text);

    // Markdown rendering helpers
    void renderBlock(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderSpan(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal lineHeight, QFont* font_passed=nullptr);
    void renderListElement(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderHeading(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderCodeBlock(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderBlockquote(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight);
    void renderTextSegment(QPainter& painter, const Element* segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal lineHeight);
    void renderTable(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal maxWidth, qreal& lineHeight);

    // Font and styling helpers
    QFont getFont(const Element* segment) const;
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

protected:
    void paintEvent(QPaintEvent* event) override;
};
