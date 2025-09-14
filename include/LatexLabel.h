#pragma once

#include <QWidget>
#include <QString>
#include <QPainter>
#include <QRect>
#include <QGuiApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QPushButton>
#include <md4c.h>
#include <vector>
#include "render.h"
#include "element.h"
#include "Fragment.h"

struct layoutInfoCodeBlock{
    int shift;
    bool isOverflowing;
    QRect boundingBox;
    int maxShift;
    QPushButton* button;
};


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
    tex::TeXRender* _render;
    QString m_text;
    QString m_raw_text; //without markdown formatting
    std::vector<Element*> m_segments;
    int m_textSize;
    double m_leading=3.0;
    Fragment* m_selected=nullptr;

    int m_curr_code_block=0;
    std::vector<layoutInfoCodeBlock> m_code_block_info;

    int margin_left=5, margin_right=5,margin_top=5,margin_bottom=5;


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


    // Debug helper method
    void printSegmentRecursive(const Element* element, int depth) const;

    // Cleanup methods for AST elements
    void cleanup_segments(std::vector<Element*>& elements);  // free all pointers in AST
    void deleteDisplayList();


    // Fragment creation helper methods for better readability
    void addText(qreal x, qreal y, qreal width, qreal height, const QString& text, const QFont& font, QPalette::ColorRole color = QPalette::Text);
    void addLatex(qreal x, qreal y, qreal width, qreal height, tex::TeXRender* render, const QString& text, bool isInline);
    void addLine(qreal x, qreal y, qreal width, qreal height, const QPoint& to, int lineWidth = 1);
    void addRoundedRect(qreal x, qreal y, qreal width, qreal height, qreal radius, QPalette::ColorRole bg, QPalette::ColorRole stroke = QPalette::WindowText);
    void addRoundedRect(qreal x, qreal y, qreal width, qreal height, qreal tl, qreal tr, qreal bl, qreal br, QPalette::ColorRole bg, QPalette::ColorRole stroke = QPalette::WindowText);
    void addClippedText(QRect clip, QRect bounding, QString& text,int shift);

protected:
    void paintEvent(QPaintEvent* event) override;
    void changeEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent *event) override;
};
