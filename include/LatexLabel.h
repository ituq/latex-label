#include <QWidget>
#include <QString>
#include <QPainter>
#include <QRect>
#include <md4c.h>
#include "latex.h"
#include "core/formula.h"
#include "render.h"

enum class LabelType {
    Heading1,     // # Heading level 1
    Heading2,     // ## Heading level 2
    Heading3,     // ### Heading level 3
    Heading4,     // #### Heading level 4
    Heading5,     // ##### Heading level 5
    Heading6,     // ###### Heading level 6
    Paragraph,    // Plain text separated by blank lines
    LineBreak,    // Two spaces at end-of-line for <br>
    Bold,         // *bold* or _bold_
    Italic,       // italic or italic
    BoldItalic,   // **bold italic**
    Blockquote,   // > blockquote
    UnorderedListItem,   // - or * or + item
    OrderedListItem,     // 1. item
    InlineCode,          // ⁠ code ⁠
    CodeBlock,           // ⁠  code  ⁠
    HorizontalRule,      // ---
    Link,                // [text](url)
    Image,               // ![alt](src)
    // Extended syntax
    Table,               // | syntax | ...
    Footnote,            // [^1]
    Strikethrough,       // ~strikethrough~
    TaskListItem,        // - [x] task
    // Add more if you need: HeadingAlt1/2, etc.
    Unknown
};

struct parsedString {
    QString text;
    LabelType type;
};

enum class TextSegmentType {
    Regular,
    InlineLatex
};

struct TextSegment {
    QString content;
    TextSegmentType type;
    tex::TeXRender* render;

    TextSegment() : render(nullptr) {}
};

class LatexLabel : public QWidget{

public:
    void appendText(QString& text);
    void setText(QString text);
    void setTextSize(int size);
    int getTextSize() const;
    LatexLabel(QWidget* parent=nullptr);
    ~LatexLabel();
private:
    std::vector<parsedString> content;
    tex::TeXRender* _render;
    QString m_text;
    std::vector<TextSegment> m_segments;
    int m_textSize;

    void parseText();
    std::vector<TextSegment> parseInlineLatex(const QString& text);
    tex::TeXRender* parseInlineLatexExpression(const QString& latex);

    // Simple incomplete LaTeX detection
    bool hasIncompleteLatexAtEnd(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
};
