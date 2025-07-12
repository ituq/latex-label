#include "LatexLabel.h"
#include "platform/qt/graphic_qt.h"
#include <QRegularExpression>
#include <QFontMetrics>
#include <QSizePolicy>
#include <md4c.h>

LatexLabel::LatexLabel(QWidget* parent) : QWidget(parent), _render(nullptr), m_textSize(12) {
    setMinimumSize(300, 100);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
}

LatexLabel::~LatexLabel(){
    if(_render != nullptr) {
        delete _render;
    }
    // Clean up segment renders
    for(auto& segment : m_segments) {
        if(segment.render != nullptr) {
            delete segment.render;
            segment.render = nullptr;
        }
    }
}

void LatexLabel::setTextSize(int size) {
    if(size != m_textSize && size > 0) {
        m_textSize = size;
        parseText(); // Reparse to update LaTeX expressions with new size
        update();    // Trigger repaint
    }
}

int LatexLabel::getTextSize() const {
    return m_textSize;
}

tex::TeXRender* LatexLabel::parseInlineLatexExpression(const QString& latex) {
    try {
        // Create a custom inline LaTeX parser using text style instead of display style
        tex::Formula formula;
        formula.setLaTeX(latex.toStdWString());

        tex::TeXRenderBuilder builder;
        tex::TeXRender* render = builder
            .setStyle(tex::TexStyle::text)
            .setTextSize(m_textSize)
            .setWidth(tex::UnitType::pixel, 280, tex::Alignment::left)
            .setIsMaxWidth(true)
            .setLineSpace(tex::UnitType::point, m_textSize)
            .setForeground(0xff424242)
            .build(formula._root);

        return render;
    } catch (const std::exception& e) {
        return nullptr;
    }
}

std::vector<TextSegment> LatexLabel::parseInlineLatex(const QString& text) {
    std::vector<TextSegment> segments;

    QRegularExpression latexRegex(R"(\$([^$]+)\$|\\\(([^)]+)\\\))");
    QRegularExpressionMatchIterator iterator = latexRegex.globalMatch(text);

    int lastEnd = 0;

    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();

        // Add regular text before the LaTeX expression
        if (match.capturedStart() > lastEnd) {
            TextSegment regularSegment;
            regularSegment.content = text.mid(lastEnd, match.capturedStart() - lastEnd);
            regularSegment.type = TextSegmentType::Regular;
            segments.push_back(regularSegment);
        }

        // Add the LaTeX expression
        TextSegment latexSegment;
        // Get the content inside the delimiters (either from $...$ or \(...\))
        latexSegment.content = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
        latexSegment.type = TextSegmentType::InlineLatex;

        // Parse the LaTeX expression using our custom inline parser
        latexSegment.render = parseInlineLatexExpression(latexSegment.content);

        if (latexSegment.render == nullptr) {
            // If LaTeX parsing fails, treat as regular text
            latexSegment.type = TextSegmentType::Regular;
            latexSegment.content = "$" + latexSegment.content + "$"; // Show original with delimiters
        }

        segments.push_back(latexSegment);
        lastEnd = match.capturedEnd();
    }

    // Add remaining regular text
    if (lastEnd < text.length()) {
        TextSegment regularSegment;
        regularSegment.content = text.mid(lastEnd);
        regularSegment.type = TextSegmentType::Regular;
        segments.push_back(regularSegment);
    }

    return segments;
}

void LatexLabel::parseText() {
    // Clean up previous segments
    for(auto& segment : m_segments) {
        if(segment.render != nullptr) {
            delete segment.render;
            segment.render = nullptr;
        }
    }
    m_segments.clear();


    // Parse the text for inline LaTeX expressions
    m_segments = parseInlineLatex(m_text);
}

void LatexLabel::paintEvent(QPaintEvent* event){
    QPainter painter(this);
    painter.setPen(Qt::black);
    painter.setFont(QFont("Zed Mono", m_textSize));
    painter.fillRect(rect(), QColor(200, 200, 200));
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFontMetricsF fontMetrics = painter.fontMetrics();
    qreal lineHeight = fontMetrics.height();
    qreal x = 5.0;
    qreal y = 5.0 + fontMetrics.ascent();  // y represents the text baseline
    qreal maxWidth = width() - 10.0;

    for(const auto& segment : m_segments) {
        if(segment.type == TextSegmentType::Regular) {
            // Render regular text with word wrapping
            QStringList words = segment.content.split(' ', Qt::SkipEmptyParts);

            for(const QString& word : words) {
                QString wordWithSpace = word + " ";
                qreal wordWidth = fontMetrics.horizontalAdvance(wordWithSpace);

                // Check if word fits on current line
                if(x + wordWidth > maxWidth && x > 5.0) {
                    // Move to next line
                    x = 5.0;
                    y += lineHeight;

                    // Check if we're still within the widget bounds
                    if(y > height() - 5.0) break;
                }

                // Draw text at baseline position y
                painter.drawText(QPointF(x, y), wordWithSpace);
                x += wordWidth;
            }
        } else if(segment.type == TextSegmentType::InlineLatex && segment.render != nullptr) {
            // Render LaTeX expression
            qreal renderWidth = segment.render->getWidth();
            qreal renderHeight = segment.render->getHeight();

            // Check if LaTeX expression fits on current line
            if(x + renderWidth > maxWidth && x > 5.0) {
                // Move to next line
                x = 5.0;
                y += lineHeight;

                // Check if we're still within the widget bounds
                if(y > height() - 5.0) break;
            }

            // Save painter state before LaTeX rendering
            painter.save();

            qreal latexHeight = segment.render->getHeight();
            qreal latexDepth = segment.render->getDepth();
            qreal latexY = y - (latexHeight - latexDepth);

            // Draw LaTeX expression
            tex::Graphics2D_qt g2(&painter);
            segment.render->draw(g2, static_cast<int>(x), static_cast<int>(latexY));

            // Restore painter state after LaTeX rendering
            painter.restore();

            // Restore our font and rendering settings
            painter.setFont(QFont("Zed Mono", m_textSize));
            painter.setRenderHint(QPainter::Antialiasing, true);

            // Move x position for next content, keep y on same line
            x += renderWidth + 3.0; // Small spacing after LaTeX
        }

        // Break if we've exceeded widget bounds
        if(y > height() - 5.0) break;
    }
}

void LatexLabel::appendText(QString& text){
    if(text.isEmpty()) return;
    static bool open_latex_block=false;
    for (QChar c: text) {
        if (c=='$') {
            open_latex_block=!open_latex_block;
        }
    }
    m_text += text;



    if(open_latex_block) {
        // Don't parse yet, wait for more text
        // Just trigger a repaint with current segments
        update();
        return;
    }

    // If we get here, we either have complete LaTeX or no LaTeX at the end
    // Reparse the entire text (much simpler and more reliable)
    parseText();
    update();
}

void LatexLabel::setText(QString text){
    m_text = text;
    parseText();
    update();
}
