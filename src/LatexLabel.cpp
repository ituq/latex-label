#include "LatexLabel.h"
#include "platform/qt/graphic_qt.h"
#include "utils/enums.h"
#include <QRegularExpression>
#include <QFontMetrics>
#include <QSizePolicy>
#include <md4c.h>

LatexLabel::LatexLabel(QWidget* parent) : QWidget(parent), _render(nullptr), m_textSize(12), m_leftMargin(5.0) {
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
        parseMarkdown(m_text); //parse again to set size for latex expressions
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

tex::TeXRender* LatexLabel::parseDisplayLatexExpression(const QString& latex) {
    try {
        // Create a display LaTeX parser using display style
        tex::Formula formula;
        formula.setLaTeX(latex.toStdWString());

        tex::TeXRenderBuilder builder;
        tex::TeXRender* render = builder
            .setStyle(tex::TexStyle::display)
            .setTextSize(m_textSize)
            .setWidth(tex::UnitType::pixel, 600, tex::Alignment::center)
            .setIsMaxWidth(true)
            .setLineSpace(tex::UnitType::point, m_textSize + 2)
            .setForeground(0xff424242)
            .build(formula._root);

        return render;
    } catch (const std::exception& e) {
        return nullptr;
    }
}

// md4c callback functions
int LatexLabel::enterBlockCallback(MD_BLOCKTYPE type, void* detail, void* userdata) {
    struct ExtendedParserState {
        MarkdownParserState* state;
        LatexLabel* label;
    };
    ExtendedParserState* extState = static_cast<ExtendedParserState*>(userdata);
    MarkdownParserState* state = extState->state;

    TextSegment segment;
    segment.type = TextSegmentType::MarkdownBlock;



    switch(type) {
        case MD_BLOCK_DOC:
            segment.blockType = MarkdownBlockType::Document;
            break;
        case MD_BLOCK_QUOTE:
            segment.blockType = MarkdownBlockType::Quote;
            segment.indentLevel = state->blockStack.size();
            break;
        case MD_BLOCK_UL:
            segment.blockType = MarkdownBlockType::UnorderedList;
            segment.indentLevel = state->blockStack.size();
            break;
        case MD_BLOCK_OL:
            segment.blockType = MarkdownBlockType::OrderedList;
            segment.indentLevel = state->blockStack.size();
            if(detail) {
                MD_BLOCK_OL_DETAIL* ol_detail = static_cast<MD_BLOCK_OL_DETAIL*>(detail);
                segment.attributes.listStartNumber = ol_detail->start;
                segment.attributes.listMarker = ol_detail->mark_delimiter;
            }
            break;
        case MD_BLOCK_LI:
            segment.blockType = MarkdownBlockType::ListItem;
            segment.indentLevel = state->blockStack.size();
            if(detail) {
                MD_BLOCK_LI_DETAIL* li_detail = static_cast<MD_BLOCK_LI_DETAIL*>(detail);
                segment.attributes.isTight = (li_detail->is_task != 0);
            }
            break;
        case MD_BLOCK_HR:
            segment.blockType = MarkdownBlockType::HorizontalRule;
            break;
        case MD_BLOCK_H:
            segment.blockType = MarkdownBlockType::Heading;
            if(detail) {
                MD_BLOCK_H_DETAIL* h_detail = static_cast<MD_BLOCK_H_DETAIL*>(detail);
                segment.attributes.headingLevel = h_detail->level;

            }
            break;
        case MD_BLOCK_CODE:
            segment.blockType = MarkdownBlockType::CodeBlock;
            if(detail) {
                MD_BLOCK_CODE_DETAIL* code_detail = static_cast<MD_BLOCK_CODE_DETAIL*>(detail);
                if(code_detail->lang.text) {
                    segment.attributes.codeLanguage = QString::fromUtf8(code_detail->lang.text, code_detail->lang.size);
                }
            }
            break;
        case MD_BLOCK_HTML:
            segment.blockType = MarkdownBlockType::HtmlBlock;
            break;
        case MD_BLOCK_P:
            segment.blockType = MarkdownBlockType::Paragraph;
            break;
        case MD_BLOCK_TABLE:
            segment.blockType = MarkdownBlockType::Table;
            break;
        case MD_BLOCK_THEAD:
            segment.blockType = MarkdownBlockType::TableHead;
            break;
        case MD_BLOCK_TBODY:
            segment.blockType = MarkdownBlockType::TableBody;
            break;
        case MD_BLOCK_TR:
            segment.blockType = MarkdownBlockType::TableRow;
            break;
        case MD_BLOCK_TH:
            segment.blockType = MarkdownBlockType::TableHeader;
            break;
        case MD_BLOCK_TD:
            segment.blockType = MarkdownBlockType::TableData;
            break;
    }

    if(!state->blockStack.empty()) {
        // Add as child to the current block
        state->blockStack.back()->children.push_back(segment);
        state->blockStack.push_back(&state->blockStack.back()->children.back());
    } else {
        // Add to top-level segments
        state->segments.push_back(segment);
        state->blockStack.push_back(&state->segments.back());
    }

    return 0;
}

int LatexLabel::leaveBlockCallback(MD_BLOCKTYPE type, void* detail, void* userdata) {
    struct ExtendedParserState {
        MarkdownParserState* state;
        LatexLabel* label;
    };
    ExtendedParserState* extState = static_cast<ExtendedParserState*>(userdata);
    MarkdownParserState* state = extState->state;

    if(!state->blockStack.empty()) {
        state->blockStack.pop_back();
    }

    return 0;
}

int LatexLabel::enterSpanCallback(MD_SPANTYPE type, void* detail, void* userdata) {
    struct ExtendedParserState {
        MarkdownParserState* state;
        LatexLabel* label;
    };
    ExtendedParserState* extState = static_cast<ExtendedParserState*>(userdata);
    MarkdownParserState* state = extState->state;

    TextSegment segment;
    segment.type = TextSegmentType::MarkdownSpan;

    switch(type) {
        case MD_SPAN_EM:
            segment.spanType = MarkdownSpanType::Emphasis;
            break;
        case MD_SPAN_STRONG:
            segment.spanType = MarkdownSpanType::Strong;
            break;
        case MD_SPAN_A:
            segment.spanType = MarkdownSpanType::Link;
            if(detail) {
                MD_SPAN_A_DETAIL* a_detail = static_cast<MD_SPAN_A_DETAIL*>(detail);
                if(a_detail->href.text) {
                    segment.attributes.url = QString::fromUtf8(a_detail->href.text, a_detail->href.size);
                }
                if(a_detail->title.text) {
                    segment.attributes.title = QString::fromUtf8(a_detail->title.text, a_detail->title.size);
                }
            }
            break;
        case MD_SPAN_IMG:
            segment.spanType = MarkdownSpanType::Image;
            if(detail) {
                MD_SPAN_IMG_DETAIL* img_detail = static_cast<MD_SPAN_IMG_DETAIL*>(detail);
                if(img_detail->src.text) {
                    segment.attributes.url = QString::fromUtf8(img_detail->src.text, img_detail->src.size);
                }
                if(img_detail->title.text) {
                    segment.attributes.title = QString::fromUtf8(img_detail->title.text, img_detail->title.size);
                }
            }
            break;
        case MD_SPAN_CODE:
            segment.spanType = MarkdownSpanType::Code;
            break;
        case MD_SPAN_DEL:
            segment.spanType = MarkdownSpanType::Strikethrough;
            break;
        case MD_SPAN_LATEXMATH:
            segment.spanType = MarkdownSpanType::LatexMath;
            break;
        case MD_SPAN_LATEXMATH_DISPLAY:
            segment.spanType = MarkdownSpanType::LatexMathDisplay;
            break;
        case MD_SPAN_WIKILINK:
            segment.spanType = MarkdownSpanType::WikiLink;
            break;
        case MD_SPAN_U:
            segment.spanType = MarkdownSpanType::Underline;
            break;
    }

    if(!state->blockStack.empty()) {
        state->blockStack.back()->children.push_back(segment);
        state->spanStack.push_back(&state->blockStack.back()->children.back());
    } else {
        state->segments.push_back(segment);
        state->spanStack.push_back(&state->segments.back());
    }

    return 0;
}

int LatexLabel::leaveSpanCallback(MD_SPANTYPE type, void* detail, void* userdata) {
    struct ExtendedParserState {
        MarkdownParserState* state;
        LatexLabel* label;
    };
    ExtendedParserState* extState = static_cast<ExtendedParserState*>(userdata);
    MarkdownParserState* state = extState->state;

    if(!state->spanStack.empty()) {
        state->spanStack.pop_back();
    }

    return 0;
}

int LatexLabel::textCallback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata) {
    struct ExtendedParserState {
        MarkdownParserState* state;
        LatexLabel* label;
    };
    ExtendedParserState* extState = static_cast<ExtendedParserState*>(userdata);
    MarkdownParserState* state = extState->state;
    LatexLabel* label = extState->label;

    QString textStr = QString::fromUtf8(text, size);

    TextSegment segment;
    segment.type = TextSegmentType::MarkdownText;
    segment.content = textStr;



    switch(type) {
        case MD_TEXT_NORMAL:
            segment.textType = MarkdownTextType::Normal;

            break;
        case MD_TEXT_NULLCHAR:
            segment.textType = MarkdownTextType::NullChar;
            segment.content = QChar(0xFFFD); // Unicode replacement character
            break;
        case MD_TEXT_BR:
            segment.textType = MarkdownTextType::HardBreak;
            break;
        case MD_TEXT_SOFTBR:
            segment.textType = MarkdownTextType::SoftBreak;
            break;
        case MD_TEXT_ENTITY:
            segment.textType = MarkdownTextType::Entity;
            break;
        case MD_TEXT_CODE:
            segment.textType = MarkdownTextType::Code;
            break;
        case MD_TEXT_HTML:
            segment.textType = MarkdownTextType::Html;
            break;
        case MD_TEXT_LATEXMATH:
            segment.textType = MarkdownTextType::LatexMath;
            break;
    }

    // Handle LaTeX math specially
    if(type == MD_TEXT_LATEXMATH) {
        // Check if we're in a display math span
        bool isDisplayMath = false;
        if(!state->spanStack.empty()) {
            TextSegment* currentSpan = state->spanStack.back();
            if(currentSpan->type == TextSegmentType::MarkdownSpan &&
               currentSpan->spanType == MarkdownSpanType::LatexMathDisplay) {
                isDisplayMath = true;
            }
        }

        if(isDisplayMath) {
            segment.render = label->parseDisplayLatexExpression(textStr);
        } else {
            segment.render = label->parseInlineLatexExpression(textStr);
        }
    }

    // Add to appropriate container
    if(!state->spanStack.empty()) {
        state->spanStack.back()->children.push_back(segment);
    } else if(!state->blockStack.empty()) {
        state->blockStack.back()->children.push_back(segment);
    } else {
        state->segments.push_back(segment);
    }

    return 0;
}

void LatexLabel::parseMarkdown(const QString& text) {
    // Clean up previous segments
    for(auto& segment : m_segments) {
        if(segment.render != nullptr) {
            delete segment.render;
            segment.render = nullptr;
        }
    }
    m_segments.clear();

    // Set up parser state
    MarkdownParserState state(m_textSize);

    // Store a reference to this LatexLabel instance in the state
    // We'll pass this through userdata but need to handle it carefully
    struct ExtendedParserState {
        MarkdownParserState* state;
        LatexLabel* label;
    };
    ExtendedParserState extendedState = { &state, this };

    // Set up md4c parser
    MD_PARSER parser = {0};
    parser.abi_version = 0;
    parser.flags = MD_FLAG_LATEXMATHSPANS | MD_FLAG_STRIKETHROUGH | MD_FLAG_UNDERLINE | MD_FLAG_TABLES | MD_FLAG_NOINDENTEDCODEBLOCKS;
    parser.enter_block = enterBlockCallback;
    parser.leave_block = leaveBlockCallback;
    parser.enter_span = enterSpanCallback;
    parser.leave_span = leaveSpanCallback;
    parser.text = textCallback;

    // Parse the markdown
    QByteArray textBytes = text.toUtf8();
    int result = md_parse(textBytes.constData(), textBytes.size(), &parser, &extendedState);

    if(result == 0) {
        m_segments = std::move(state.segments);
    } else {
        // Fallback to regex parsing for LaTeX expressions
        TextSegment fallback;
        fallback.content=text;
        m_segments = {fallback};
    }
}


QFont LatexLabel::getFont(const TextSegment& segment, const QFont& parentFont) const {
    QFont font = parentFont;

    switch(segment.type) {
        case TextSegmentType::MarkdownBlock:
            if(segment.blockType == MarkdownBlockType::Heading) {
                int headingSize = std::max(8, m_textSize + (6 - segment.attributes.headingLevel) * 4);
                font.setPointSize(headingSize);
                font.setBold(true);
            } else if(segment.blockType == MarkdownBlockType::CodeBlock) {
                font.setFamily("Monaco");
            }
            break;
        case TextSegmentType::MarkdownSpan:
            if(segment.spanType == MarkdownSpanType::Strong) {
                font.setBold(true);
            } else if(segment.spanType == MarkdownSpanType::Emphasis) {
                font.setItalic(true);
            } else if(segment.spanType == MarkdownSpanType::Code) {
                font.setFamily("Monaco");
            } else if(segment.spanType == MarkdownSpanType::Strikethrough) {
                font.setStrikeOut(true);
            } else if(segment.spanType == MarkdownSpanType::Underline) {
                font.setUnderline(true);
            }
            break;
        case TextSegmentType::MarkdownText:
            if(segment.textType == MarkdownTextType::Code) {
                font.setFamily("Monaco");
            }
            break;
        default:
            break;
    }

    return font;
}

QFont LatexLabel::getFont(const TextSegment& segment) const {
    QFont baseFont("Arial", m_textSize);
    return getFont(segment, baseFont);
}


qreal LatexLabel::getLineHeight(const TextSegment& segment, const QFontMetricsF& metrics) const {
    switch(segment.type) {
        case TextSegmentType::MarkdownBlock:
            if(segment.blockType == MarkdownBlockType::Heading) {
                return metrics.height() * 1.2; // Extra spacing for headings
            } else if(segment.blockType == MarkdownBlockType::CodeBlock) {
                return metrics.height() * 1.1; // Slight extra spacing for code blocks
            }
            break;
        default:
            break;
    }

    return metrics.height();
}

void LatexLabel::renderTextSegment(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal lineHeight, const QFont& parentFont) {
    QFont font = getFont(segment, parentFont);
    QColor color = segment.spanType==MarkdownSpanType::Link ? Qt::blue : Qt::black;

    painter.setFont(font);
    painter.setPen(color);

    QFontMetricsF metrics(font);
    //Remember the left margin for line wrapping
    //m_leftMargin = x;


    // Handle different text types

    if(segment.textType == MarkdownTextType::HardBreak) {
        y += lineHeight;
        return;
    } else if(segment.textType == MarkdownTextType::SoftBreak) {
        // Soft breaks should create a new line in markdown
        //y += lineHeight;
        return;
    }


    // Handle LaTeX math
    if(segment.type == TextSegmentType::MarkdownText && segment.textType == MarkdownTextType::LatexMath && segment.render != nullptr) {
        qreal renderWidth = segment.render->getWidth();
        qreal renderHeight = segment.render->getHeight();

        //Check if LaTeX expression fits on current line
        if(x + renderWidth > maxWidth && x > m_leftMargin) {
            x = m_leftMargin;
            y += lineHeight;
        }

        // Draw LaTeX expression
        painter.save();
        qreal latexY = y - (renderHeight - segment.render->getDepth());
        tex::Graphics2D_qt g2(&painter);
        segment.render->draw(g2, static_cast<int>(x), static_cast<int>(latexY));
        painter.restore();

        x += renderWidth + 3.0;
        return;
    }

    //regular text
    QString text = segment.content;



    QStringList words = text.split(' ', Qt::SkipEmptyParts);
    for(const QString& word : words) {
        if(word=="\n"){
            x = m_leftMargin;
            y += lineHeight;
            continue;
        }
        QString wordWithSpace = word + " ";
        qreal wordWidth = metrics.horizontalAdvance(wordWithSpace);

        //Check if word fits on current line
        if(x + wordWidth > maxWidth && x > m_leftMargin) {
            x = m_leftMargin;
            y += lineHeight;
        }

        painter.drawText(QPointF(x, y), wordWithSpace);
        x += wordWidth;
    }
}

void LatexLabel::renderTextSegment(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal lineHeight) {
    QFont baseFont("Arial", m_textSize);
    renderTextSegment(painter, segment, x, y, maxWidth, lineHeight, baseFont);
}

void LatexLabel::renderBlockElement(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal& lineHeight) {
    QFont font = getFont(segment);
    QFontMetricsF metrics(font);
    qreal currentLineHeight = getLineHeight(segment, metrics);

    switch(segment.blockType) {
        case MarkdownBlockType::Document:
            // Document block - render all children
            for(const auto& child : segment.children) {
                if(child.type == TextSegmentType::MarkdownBlock) {
                    renderBlockElement(painter, child, x, y, maxWidth, lineHeight);
                } else if(child.type == TextSegmentType::MarkdownSpan) {
                    renderSpanElement(painter, child, x, y, maxWidth, lineHeight);
                } else if(child.type == TextSegmentType::MarkdownText) {
                    renderTextSegment(painter, child, x, y, maxWidth, lineHeight);
                }
            }
            break;
        case MarkdownBlockType::Heading:
            renderHeading(painter, segment, x, y, maxWidth, lineHeight);
            break;
        case MarkdownBlockType::CodeBlock:
            renderCodeBlock(painter, segment, x, y, maxWidth, lineHeight);
            break;
        case MarkdownBlockType::Quote:
            renderBlockquote(painter, segment, x, y, maxWidth, lineHeight);
            break;
        case MarkdownBlockType::UnorderedList:
        case MarkdownBlockType::OrderedList:
        case MarkdownBlockType::ListItem:
            renderListElement(painter, segment, x, y, maxWidth, lineHeight);
            break;
        case MarkdownBlockType::Table:
            renderTable(painter, segment, x, y, maxWidth, lineHeight);
            break;
        case MarkdownBlockType::HorizontalRule:
            // Draw horizontal line
            painter.save();
            painter.setPen(QPen(Qt::gray, 2));
            painter.drawLine(5, y, maxWidth, y);
            painter.restore();
            y += 10;
            break;
        case MarkdownBlockType::Paragraph:
            //Regular paragraph - render children with base font
            {
                QFont baseFont("Arial", m_textSize);
                m_leftMargin = x; //Remember the current indentation level
                for(const auto& child : segment.children) {
                    if(child.type == TextSegmentType::MarkdownSpan) {
                        renderSpanElement(painter, child, x, y, maxWidth, lineHeight, baseFont);
                    } else if(child.type == TextSegmentType::MarkdownText) {
                        renderTextSegment(painter, child, x, y, maxWidth, lineHeight, baseFont);
                    }
                }
                x = m_leftMargin;
                y += currentLineHeight;
            }
            break;
        default:
            // Default block rendering
            {
                QFont baseFont("Arial", m_textSize);
                for(const auto& child : segment.children) {
                    if(child.type == TextSegmentType::MarkdownSpan) {
                        renderSpanElement(painter, child, x, y, maxWidth, lineHeight, baseFont);
                    } else if(child.type == TextSegmentType::MarkdownText) {
                        renderTextSegment(painter, child, x, y, maxWidth, lineHeight, baseFont);
                    }
                }
            }
            break;
    }
}

void LatexLabel::renderSpanElement(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal lineHeight, const QFont& parentFont) {
    // Apply span-specific styling and render children
    painter.save();

    QFont font = getFont(segment, parentFont);
    QColor color = segment.spanType==MarkdownSpanType::Link ? Qt::blue : Qt::black;
    painter.setFont(font);
    painter.setPen(color);


    // Handle special spans
    if(segment.spanType == MarkdownSpanType::LatexMath || segment.spanType == MarkdownSpanType::LatexMathDisplay) {
        // Need to find the LaTeX content in the children and render it
        for(const auto& child : segment.children) {
            if(child.type == TextSegmentType::MarkdownText && child.textType == MarkdownTextType::LatexMath) {
                // Parse and render the LaTeX content
                tex::TeXRender* latexRender = nullptr;
                if(segment.spanType == MarkdownSpanType::LatexMathDisplay) {
                    latexRender = const_cast<LatexLabel*>(this)->parseDisplayLatexExpression(child.content);
                } else {
                    latexRender = const_cast<LatexLabel*>(this)->parseInlineLatexExpression(child.content);
                }

                if(latexRender != nullptr) {
                    qreal renderWidth = latexRender->getWidth();
                    qreal renderHeight = latexRender->getHeight();

                    //Check if LaTeX expression fits on current line
                    if(x + renderWidth > maxWidth && x > m_leftMargin) {
                        x = m_leftMargin;
                        y += lineHeight;
                    }

                    // Draw LaTeX expression
                    qreal latexY = y - (renderHeight - latexRender->getDepth());
                    tex::Graphics2D_qt g2(&painter);
                    latexRender->draw(g2, static_cast<int>(x), static_cast<int>(latexY));

                    x += renderWidth + 3.0;
                    // Don't change y position - keep the baseline consistent

                    // Clean up the temporary render
                    delete latexRender;
                }
            }
        }
    } else {
        // Render text children with inherited font
        for(const auto& child : segment.children) {
            if(child.type == TextSegmentType::MarkdownText) {
                renderTextSegment(painter, child, x, y, maxWidth, lineHeight, font);
            } else if(child.type == TextSegmentType::MarkdownSpan) {
                renderSpanElement(painter, child, x, y, maxWidth, lineHeight, font);
            }
        }
    }

    painter.restore();
}

void LatexLabel::renderSpanElement(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal lineHeight) {
    QFont baseFont("Arial", m_textSize);
    renderSpanElement(painter, segment, x, y, maxWidth, lineHeight, baseFont);
}

void LatexLabel::renderListElement(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal& lineHeight) {
    qreal indent = segment.indentLevel * 20.0;
    qreal originalX = x;

    if(segment.blockType == MarkdownBlockType::ListItem) {
        // Draw list marker
        painter.save();
        QFont baseFont("Arial", m_textSize);
        painter.setFont(baseFont);
        painter.setPen(Qt::black);

        x = 5.0 + indent;
        painter.drawText(QPointF(x, y), "• ");
        x += 15.0;

        painter.restore();

        // Render list item content
        QFont listFont("Arial", m_textSize);
        for(const auto& child : segment.children) {
            if(child.type == TextSegmentType::MarkdownSpan) {
                renderSpanElement(painter, child, x, y, maxWidth - indent, lineHeight, listFont);
            } else if(child.type == TextSegmentType::MarkdownText) {
                renderTextSegment(painter, child, x, y, maxWidth - indent, lineHeight, listFont);
            } else if(child.type == TextSegmentType::MarkdownBlock) {
                renderBlockElement(painter, child, x, y, maxWidth - indent, lineHeight);
            }
        }

        y += lineHeight * 1.3; // Add extra spacing between list items
        x = originalX;
    } else {
        // List container - render children
        for(const auto& child : segment.children) {
            if(child.type == TextSegmentType::MarkdownBlock) {
                renderBlockElement(painter, child, x, y, maxWidth, lineHeight);
            }
        }
        // Add spacing after the entire list
        y += lineHeight * 0.5;
    }
}

void LatexLabel::renderHeading(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal& lineHeight) {
    painter.save();

    QFont font = getFont(segment);
    QFontMetricsF metrics(font);
    painter.setFont(font);
    painter.setPen(Qt::black);

    qreal headingLineHeight = getLineHeight(segment, metrics);

    // Add more spacing before heading (move to next line if not at start)
    if(x > 5.0) {
        x = 5.0;
        y += lineHeight;
    }
    y += headingLineHeight * 0.8; // Increased spacing before heading

    x = 5.0;

    // Render heading content with heading font inherited
    for(const auto& child : segment.children) {
        if(child.type == TextSegmentType::MarkdownSpan) {
            renderSpanElement(painter, child, x, y, maxWidth, headingLineHeight, font);
        } else if(child.type == TextSegmentType::MarkdownText) {
            renderTextSegment(painter, child, x, y, maxWidth, headingLineHeight, font);
        }
    }

    // Add spacing after heading and move to next line
    x = 5.0;
    y += headingLineHeight * 0.8; // Increased spacing after heading

    painter.restore();
}

void LatexLabel::renderCodeBlock(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal& lineHeight) {
    painter.save();

    // Set code font
    QFont font = getFont(segment);
    QFontMetricsF fm(font);
    painter.setFont(font);
    painter.setPen(Qt::white);

    x = 10.0; // Indent code block
    y += 10.0;
    double startY = y;

    // Measure total height first
    qreal tempY = y;
    for(const auto& child : segment.children) {
        if(child.type == TextSegmentType::MarkdownText) {
            QString text = child.content;
            QStringList lines = text.split('\n');
            tempY += lines.size() * lineHeight;
        }
    }

    // Draw background first

    painter.fillRect(QRectF(5, startY-fm.ascent(), maxWidth - 10, tempY - startY), Qt::black);

    // Draw text
    for(const auto& child : segment.children) {
        if(child.type == TextSegmentType::MarkdownText) {
            QString text = child.content;
            QStringList lines = text.split('\n');

            for(const QString& line : lines) {
                painter.drawText(QPointF(x, y), line);
                y += lineHeight;
            }
        }
    }

    y += 10.0; // Bottom margin
    x = 5.0;

    painter.restore();
}

void LatexLabel::renderBlockquote(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal& lineHeight) {
    painter.save();
    // Add spacing before blockquote and move to next line if not at start
    y += lineHeight * 0.5; // Add spacing before blockquote

    // Indent content
    x = 50;
    double savedMargin=m_leftMargin;
    m_leftMargin=x;

    // Record starting position for border
    qreal startY = y;
    qreal startX = x;

    // Render blockquote content
    QFont blockquoteFont("Arial", m_textSize);
    QFontMetricsF metrics(blockquoteFont);
    for(const auto& child : segment.children) {
        if(child.type == TextSegmentType::MarkdownBlock) {
            renderBlockElement(painter, child, x, y, maxWidth - 50, lineHeight);
        } else if(child.type == TextSegmentType::MarkdownSpan) {
            renderSpanElement(painter, child, x, y, maxWidth - 50, lineHeight, blockquoteFont);
        } else if(child.type == TextSegmentType::MarkdownText) {
            renderTextSegment(painter, child, x, y, maxWidth - 50, lineHeight, blockquoteFont);
        }
    }

    //draw line left of quote
    painter.setPen(QPen(Qt::gray, 3));
    painter.drawLine(startX-5, startY - metrics.ascent(), startX-5, y-metrics.height() + metrics.descent()); // Draw from start to end of content

    // Reset position and add spacing after blockquote
    x = 5.0;
    y += lineHeight * 1.2; // Add more spacing after blockquote

    painter.restore();
    m_leftMargin=savedMargin;
}

void LatexLabel::renderTable(QPainter& painter, const TextSegment& segment, qreal& x, qreal& y, qreal maxWidth, qreal& lineHeight) {
    // Simple table rendering - just render content for now
    for(const auto& child : segment.children) {
        if(child.type == TextSegmentType::MarkdownBlock) {
            renderBlockElement(painter, child, x, y, maxWidth, lineHeight);
        }
    }
}

void LatexLabel::paintEvent(QPaintEvent* event){
    QPainter painter(this);
    painter.fillRect(rect(), QColor(255, 255, 255)); // White background
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFontMetricsF fontMetrics = painter.fontMetrics();
    qreal lineHeight = fontMetrics.height() * 1.2;
    qreal x = 5.0;
    qreal y = 5.0 + fontMetrics.ascent();  // y represents the text baseline
    qreal maxWidth = width() - 10.0;

    for(const auto& segment : m_segments) {
        // Check if we're still within the widget bounds
        if(y > height() - 5.0) break;

        switch(segment.type) {

            case TextSegmentType::MarkdownBlock:
                renderBlockElement(painter, segment, x, y, maxWidth, lineHeight);
                break;

            case TextSegmentType::MarkdownSpan:
                renderSpanElement(painter, segment, x, y, maxWidth, lineHeight);
                break;

            case TextSegmentType::MarkdownText:
                renderTextSegment(painter, segment, x, y, maxWidth, lineHeight);
                break;
        }
    }
}

void LatexLabel::appendText(MD_TEXTTYPE type, QString& text){
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
    parseMarkdown(m_text);
    update();
    adjustSize();
}

void LatexLabel::appendText(QString& text){
    // Legacy method - call the new one with MD_TEXT_NORMAL
    appendText(MD_TEXT_NORMAL, text);
}

void LatexLabel::setText(QString text){
    m_text = text;
    parseMarkdown(m_text);
    update();
    adjustSize();
}
