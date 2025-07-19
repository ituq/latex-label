#include "LatexLabel.h"
#include "platform/qt/graphic_qt.h"
#include "utils/enums.h"
#include <QRegularExpression>
#include <QFontMetrics>
#include <QSizePolicy>
#include <QDebug>
#include <md4c.h>
#include <functional>

static double widget_height=200;

LatexLabel::LatexLabel(QWidget* parent) : QWidget(parent), _render(nullptr), m_textSize(12), m_leftMargin(5.0) {

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QFontMetricsF base_metric(QFont("Arial",m_textSize));
    widget_height=300; // Start with a reasonable height
    setMinimumHeight(widget_height);
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
QSize LatexLabel::sizeHint() const{
    //Return a flexible size hint that works well with scroll areas
    return QSize(400, widget_height);
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

    QString blockTypeName;
    switch(type) {
        case MD_BLOCK_DOC: blockTypeName = "DOC"; break;
        case MD_BLOCK_QUOTE: blockTypeName = "QUOTE"; break;
        case MD_BLOCK_UL: blockTypeName = "UL"; break;
        case MD_BLOCK_OL: blockTypeName = "OL"; break;
        case MD_BLOCK_LI: blockTypeName = "LI"; break;
        case MD_BLOCK_P: blockTypeName = "P"; break;
        case MD_BLOCK_H: blockTypeName = "H"; break;
        default: blockTypeName = QString("UNKNOWN_%1").arg(type); break;
    }

    switch(type) {
        case MD_BLOCK_DOC:
            segment.blockType = MarkdownBlockType::Document;
            break;
        case MD_BLOCK_QUOTE:
            segment.blockType = MarkdownBlockType::Quote;
            segment.indentLevel = 0; //quotes don't use list indentation
            break;
        case MD_BLOCK_UL:
            segment.blockType = MarkdownBlockType::UnorderedList;
            segment.indentLevel = state->list_nesting_level;
            qDebug() << "ENTER UL: nesting_level =" << state->list_nesting_level << "indent_level =" << segment.indentLevel;
            state->list_nesting_level++; //increment for nested lists
            state->list_type_stack.push_back(MarkdownBlockType::UnorderedList);
            state->list_item_counters.push_back(0); //unordered lists don't need counting
            break;
        case MD_BLOCK_OL:
            segment.blockType = MarkdownBlockType::OrderedList;
            segment.indentLevel = state->list_nesting_level;
            qDebug() << "ENTER OL: nesting_level =" << state->list_nesting_level << "indent_level =" << segment.indentLevel;
            state->list_nesting_level++; //increment for nested lists
            state->list_type_stack.push_back(MarkdownBlockType::OrderedList);
            if(detail) {
                MD_BLOCK_OL_DETAIL* ol_detail = static_cast<MD_BLOCK_OL_DETAIL*>(detail);
                segment.attributes.listStartNumber = ol_detail->start;
                segment.attributes.listMarker = ol_detail->mark_delimiter;
                state->list_item_counters.push_back(ol_detail->start); //start counting from the specified number
            } else {
                state->list_item_counters.push_back(1); //default start from 1
            }
            break;
        case MD_BLOCK_LI:
            segment.blockType = MarkdownBlockType::ListItem;
            segment.indentLevel = state->list_nesting_level - 1; //list items use parent list's level
            qDebug() << "ENTER LI: nesting_level =" << state->list_nesting_level << "indent_level =" << segment.indentLevel;

            //store parent list type and current item number
            if(!state->list_type_stack.empty()) {
                segment.attributes.parentListType = state->list_type_stack.back();
                if(segment.attributes.parentListType == MarkdownBlockType::OrderedList && !state->list_item_counters.empty()) {
                    segment.attributes.listItemNumber = state->list_item_counters.back();
                    state->list_item_counters.back()++; //increment for next item
                    qDebug() << "  LI item_number =" << segment.attributes.listItemNumber;
                }
                qDebug() << "  LI parent_type =" << (segment.attributes.parentListType == MarkdownBlockType::OrderedList ? "OL" : "UL");
            }

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
        qDebug() << "Adding" << blockTypeName << "as child to" << (int)state->blockStack.back()->blockType;

        // Check if this list should be nested inside a previous list item instead of the current parent
        bool shouldNestInLastLI = false;
        TextSegment* lastLI = nullptr;

        if((type == MD_BLOCK_UL || type == MD_BLOCK_OL) &&
           state->blockStack.back()->blockType == MarkdownBlockType::Document) {
            qDebug() << "Checking for nesting in Document:" << blockTypeName;

            // Only check for nesting if the last child is a list or list item
            // This prevents nesting when there's intervening content like paragraphs
            auto& documentChildren = state->blockStack.back()->children;
            if(!documentChildren.empty()) {
                auto& lastChild = documentChildren.back();

                // Only consider nesting if the last element is a list-related block
                if(lastChild.type == TextSegmentType::MarkdownBlock &&
                   (lastChild.blockType == MarkdownBlockType::OrderedList ||
                    lastChild.blockType == MarkdownBlockType::UnorderedList ||
                    lastChild.blockType == MarkdownBlockType::ListItem)) {

                    // Look for the most recent complete list and get its last item
                    std::function<std::pair<TextSegment*, TextSegment*>(TextSegment&)> findLastCompleteList = [&](TextSegment& seg) -> std::pair<TextSegment*, TextSegment*> {
                        // Check if this segment itself is a list
                        if(seg.type == TextSegmentType::MarkdownBlock &&
                           (seg.blockType == MarkdownBlockType::OrderedList || seg.blockType == MarkdownBlockType::UnorderedList)) {
                            // Found a list, look for its last list item
                            for(auto it = seg.children.rbegin(); it != seg.children.rend(); ++it) {
                                if(it->type == TextSegmentType::MarkdownBlock && it->blockType == MarkdownBlockType::ListItem) {
                                    return std::make_pair(&seg, &(*it));
                                }
                            }
                        }
                        // Check children in reverse order for lists
                        for(auto it = seg.children.rbegin(); it != seg.children.rend(); ++it) {
                            if(auto result = findLastCompleteList(*it); result.first != nullptr) {
                                return result;
                            }
                        }
                        return std::make_pair(nullptr, nullptr);
                    };

                    // Check the Document for lists
                    auto result = findLastCompleteList(*state->blockStack.back());
                    auto lastList = result.first;
                    auto lastListItem = result.second;
                    qDebug() << "findLastCompleteList result: lastList=" << (lastList ? "found" : "null")
                             << "lastListItem=" << (lastListItem ? "found" : "null");
                    if(lastList && lastListItem) {
                        // Only nest if the new list type is different from the parent list type
                        MarkdownBlockType newListType = (type == MD_BLOCK_UL) ? MarkdownBlockType::UnorderedList : MarkdownBlockType::OrderedList;
                        qDebug() << "lastList blockType:" << (int)lastList->blockType << "newListType:" << (int)newListType;
                        if(lastList->blockType != newListType) {
                            lastLI = lastListItem;
                            shouldNestInLastLI = true;
                            qDebug() << "Setting shouldNestInLastLI = true";
                        }
                    }
                }
            }
        }

        if(shouldNestInLastLI && lastLI) {
            // Add as child to the last list item and adjust indent level
            qDebug() << "NESTING" << blockTypeName << "inside previous LI";
            segment.indentLevel = lastLI->indentLevel + 1;
            // Adjust list_nesting_level to reflect deeper nesting
            state->list_nesting_level = segment.indentLevel;
            lastLI->children.push_back(segment);
            state->blockStack.push_back(&lastLI->children.back());
        } else {
            // Add as child to the current block
            state->blockStack.back()->children.push_back(segment);
            state->blockStack.push_back(&state->blockStack.back()->children.back());
        }
    } else {
        qDebug() << "blockStack empty for" << blockTypeName << "segments count:" << state->segments.size();
        // Check if this list should be nested inside the last list item
        bool shouldNestInLastLI = false;
        TextSegment* lastLI = nullptr;

        if((type == MD_BLOCK_UL || type == MD_BLOCK_OL) && !state->segments.empty()) {
            qDebug() << "Checking for nesting:" << blockTypeName << "segments count:" << state->segments.size();

            // Only check for nesting if the last segment is a list or list item
            // This prevents nesting when there's intervening content like paragraphs
            auto& lastSegment = state->segments.back();
            qDebug() << "lastSegment blockType:" << (int)lastSegment.blockType;

            // Only consider nesting if the last element is a list-related block
            if(lastSegment.type == TextSegmentType::MarkdownBlock &&
               (lastSegment.blockType == MarkdownBlockType::OrderedList ||
                lastSegment.blockType == MarkdownBlockType::UnorderedList ||
                lastSegment.blockType == MarkdownBlockType::ListItem)) {

                // Look for the most recent complete list and get its last item
                std::function<std::pair<TextSegment*, TextSegment*>(TextSegment&)> findLastCompleteList = [&](TextSegment& seg) -> std::pair<TextSegment*, TextSegment*> {
                    // Check if this segment itself is a list
                    if(seg.type == TextSegmentType::MarkdownBlock &&
                       (seg.blockType == MarkdownBlockType::OrderedList || seg.blockType == MarkdownBlockType::UnorderedList)) {
                        // Found a list, look for its last list item
                        for(auto it = seg.children.rbegin(); it != seg.children.rend(); ++it) {
                            if(it->type == TextSegmentType::MarkdownBlock && it->blockType == MarkdownBlockType::ListItem) {
                                return std::make_pair(&seg, &(*it));
                            }
                        }
                    }
                    // Check children in reverse order for lists
                    for(auto it = seg.children.rbegin(); it != seg.children.rend(); ++it) {
                        if(auto result = findLastCompleteList(*it); result.first != nullptr) {
                            return result;
                        }
                    }
                    return std::make_pair(nullptr, nullptr);
                };

                auto result = findLastCompleteList(lastSegment);
                auto lastList = result.first;
                auto lastListItem = result.second;
                qDebug() << "findLastCompleteList result: lastList=" << (lastList ? "found" : "null")
                         << "lastListItem=" << (lastListItem ? "found" : "null");
                if(lastList && lastListItem) {
                    // Only nest if the new list type is different from the parent list type
                    MarkdownBlockType newListType = (type == MD_BLOCK_UL) ? MarkdownBlockType::UnorderedList : MarkdownBlockType::OrderedList;
                    qDebug() << "lastList blockType:" << (int)lastList->blockType << "newListType:" << (int)newListType;
                    if(lastList->blockType != newListType) {
                        lastLI = lastListItem;
                        shouldNestInLastLI = true;
                        qDebug() << "Setting shouldNestInLastLI = true";
                    }
                }
            }
        }

        if(shouldNestInLastLI && lastLI) {
            // Add as child to the last list item and adjust indent level
            qDebug() << "NESTING" << blockTypeName << "inside previous LI";
            segment.indentLevel = lastLI->indentLevel + 1;
            // Adjust list_nesting_level to reflect deeper nesting
            state->list_nesting_level = segment.indentLevel;
            lastLI->children.push_back(segment);
            state->blockStack.push_back(&lastLI->children.back());
        } else {
            // Add to top-level segments
            state->segments.push_back(segment);
            state->blockStack.push_back(&state->segments.back());
        }
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

    QString blockTypeName;
    switch(type) {
        case MD_BLOCK_DOC: blockTypeName = "DOC"; break;
        case MD_BLOCK_QUOTE: blockTypeName = "QUOTE"; break;
        case MD_BLOCK_UL: blockTypeName = "UL"; break;
        case MD_BLOCK_OL: blockTypeName = "OL"; break;
        case MD_BLOCK_LI: blockTypeName = "LI"; break;
        case MD_BLOCK_P: blockTypeName = "P"; break;
        case MD_BLOCK_H: blockTypeName = "H"; break;
        default: blockTypeName = QString("UNKNOWN_%1").arg(type); break;
    }

    //decrement list nesting level when leaving list containers
    if(type == MD_BLOCK_UL || type == MD_BLOCK_OL) {
        qDebug() << "LEAVE" << (type == MD_BLOCK_UL ? "UL" : "OL") << ": nesting_level before decrement =" << state->list_nesting_level;
        if(state->list_nesting_level > 0) {
            state->list_nesting_level--;
        }
        qDebug() << "  nesting_level after decrement =" << state->list_nesting_level;
        //pop from list type and counter stacks
        if(!state->list_type_stack.empty()) {
            state->list_type_stack.pop_back();
        }
        if(!state->list_item_counters.empty()) {
            state->list_item_counters.pop_back();
        }
    } else if(type == MD_BLOCK_LI) {
        qDebug() << "LEAVE LI: nesting_level =" << state->list_nesting_level;
    }

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
    parser.flags = MD_FLAG_LATEXMATHSPANS | MD_FLAG_STRIKETHROUGH | MD_FLAG_UNDERLINE | MD_FLAG_TABLES;
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
        y += lineHeight+m_leading;
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
            y += lineHeight+m_leading;
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
            y += lineHeight +m_leading;
            continue;
        }
        QString wordWithSpace = word + " ";
        qreal wordWidth = metrics.horizontalAdvance(wordWithSpace);

        //Check if word fits on current line
        if(x + wordWidth > maxWidth && x > m_leftMargin) {
            x = m_leftMargin;
            y += lineHeight+m_leading;
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
            y += 2*lineHeight;
            painter.save();
            painter.setPen(QPen(Qt::gray, 2));
            painter.drawLine(5, y, maxWidth, y);
            painter.restore();
            y += 2*lineHeight;
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

                    if(segment.spanType == MarkdownSpanType::LatexMathDisplay){
                        y += renderHeight;
                        x=maxWidth/2-renderWidth/2;
                    }
                    else if(x + renderWidth > maxWidth && x > m_leftMargin){
                        //inline latex, check if it fits on line
                        x = m_leftMargin;
                        y += lineHeight+m_leading;
                    }


                    // Draw LaTeX expression
                    qreal latexY = y - (renderHeight - latexRender->getDepth());
                    tex::Graphics2D_qt g2(&painter);
                    latexRender->draw(g2, static_cast<int>(x), static_cast<int>(latexY));


                    if(segment.spanType == MarkdownSpanType::LatexMathDisplay){
                        x=m_leftMargin;
                        y += renderHeight;
                    }
                    else{
                        x += renderWidth + 3.0;
                    }

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
    //Convert indent level to actual pixels (reasonable indentation for nested lists)
    qreal indentPixels = segment.indentLevel * 16.0;
    qreal originalX = x;

    //Apply indentation to x position
    x += indentPixels;

    if(segment.blockType == MarkdownBlockType::ListItem) {
        // Draw list marker
        painter.save();
        QFont baseFont("Arial", m_textSize);
        painter.setFont(baseFont);
        painter.setPen(Qt::black);

        //determine marker based on parent list type
        QString marker;
        if(segment.attributes.parentListType == MarkdownBlockType::OrderedList) {
            //ordered list - use item number
            marker = QString::number(segment.attributes.listItemNumber) + ". ";
        } else {
            //unordered list - use bullet
            marker = "• ";
        }


        painter.drawText(QPointF(x, y), marker);

        //adjust x position based on marker width
        QFontMetricsF metrics(baseFont);
        qreal markerWidth = metrics.horizontalAdvance(marker);
        x += markerWidth + 2.0;

        painter.restore();

        //Update left margin so text wrapping respects list indentation
        qreal savedMargin = m_leftMargin;
        m_leftMargin = x;

        // Render list item content
        QFont listFont("Arial", m_textSize);
        for(const auto& child : segment.children) {
            if(child.type == TextSegmentType::MarkdownSpan) {
                renderSpanElement(painter, child, x, y, maxWidth - indentPixels, lineHeight, listFont);
            } else if(child.type == TextSegmentType::MarkdownText) {
                renderTextSegment(painter, child, x, y, maxWidth - indentPixels, lineHeight, listFont);
            } else if(child.type == TextSegmentType::MarkdownBlock && (child.blockType== MarkdownBlockType::OrderedList ||child.blockType== MarkdownBlockType::UnorderedList)) {
                y+=lineHeight * 1.3;

                renderBlockElement(painter, child, x, y, maxWidth - indentPixels, lineHeight);
            } else if(child.type == TextSegmentType::MarkdownBlock) {
                qreal nestedX = originalX+markerWidth;
                renderBlockElement(painter, child, x, y, maxWidth - indentPixels, lineHeight);
            }
        }
        y += lineHeight * 1.3;
        x = originalX;

        //Restore original left margin
        m_leftMargin = savedMargin;
    } else {
        // List container - render children
        for(const auto& child : segment.children) {
            if(child.type == TextSegmentType::MarkdownBlock) {
                renderBlockElement(painter, child, x, y, maxWidth, lineHeight);
            }
        }
        // Add spacing after the entire list
        x=originalX;
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
            tempY += lines.size() * (lineHeight+m_leading);
        }
    }

    // Draw background first
    painter.setPen(Qt::gray);
    painter.setBrush(Qt::white);

    painter.drawRoundedRect(QRectF(5, startY-fm.ascent(), maxWidth - 10, tempY - startY),10,10);
    painter.setPen(Qt::black);


    x+=10;
    y+=10;
    // Draw text
    for(const auto& child : segment.children) {
        if(child.type == TextSegmentType::MarkdownText) {
            QString text = child.content;
            QStringList lines = text.split('\n');

            for(const QString& line : lines) {
                painter.drawText(QPointF(x, y), line);
                y += lineHeight+m_leading;
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
    //Convert markdown table to LaTeX table syntax
    y+=lineHeight;
    QString latexTable;
    int columnCount = 0;
    bool hasHeader = false;

    //First pass: determine column count and structure
    for(const auto& child : segment.children) {
        if(child.type == TextSegmentType::MarkdownBlock) {
            if(child.blockType == MarkdownBlockType::TableHead) {
                hasHeader = true;
                //Find first row to determine column count
                for(const auto& headChild : child.children) {
                    if(headChild.type == TextSegmentType::MarkdownBlock &&
                       headChild.blockType == MarkdownBlockType::TableRow) {
                        for(const auto& rowChild : headChild.children) {
                            if(rowChild.type == TextSegmentType::MarkdownBlock &&
                               rowChild.blockType == MarkdownBlockType::TableHeader) {
                                columnCount++;
                            }
                        }
                        break; //only count first row
                    }
                }
            } else if(child.blockType == MarkdownBlockType::TableBody && columnCount == 0) {
                //fallback: count columns from first data row if no header
                for(const auto& bodyChild : child.children) {
                    if(bodyChild.type == TextSegmentType::MarkdownBlock &&
                       bodyChild.blockType == MarkdownBlockType::TableRow) {
                        for(const auto& rowChild : bodyChild.children) {
                            if(rowChild.type == TextSegmentType::MarkdownBlock &&
                               rowChild.blockType == MarkdownBlockType::TableData) {
                                columnCount++;
                            }
                        }
                        break; //only count first row
                    }
                }
            }
        }
    }

    if(columnCount == 0) {
        //fallback to simple rendering if table structure can't be determined
        for(const auto& child : segment.children) {
            if(child.type == TextSegmentType::MarkdownBlock) {
                renderBlockElement(painter, child, x, y, maxWidth, lineHeight);
            }
        }
        return;
    }

    //Build LaTeX table
    latexTable = "\\begin{array}{|";
    for(int i = 0; i < columnCount; i++) {
        latexTable += "c|";
    }
    latexTable += "}\n\\hline\n";

    //extract table content
    std::function<QString(const TextSegment&)> extractTextContent = [&](const TextSegment& seg) -> QString {
        QString content;
        if(seg.type == TextSegmentType::MarkdownText) {
            content += seg.content;
        }
        for(const auto& child : seg.children) {
            content += extractTextContent(child);
        }
        return content.trimmed();
    };

    //process table rows
    for(const auto& child : segment.children) {
        if(child.type == TextSegmentType::MarkdownBlock) {
            if(child.blockType == MarkdownBlockType::TableHead) {
                //process header rows
                for(const auto& headChild : child.children) {
                    if(headChild.type == TextSegmentType::MarkdownBlock &&
                       headChild.blockType == MarkdownBlockType::TableRow) {
                        QStringList cellContents;
                        for(const auto& rowChild : headChild.children) {
                            if(rowChild.type == TextSegmentType::MarkdownBlock &&
                               rowChild.blockType == MarkdownBlockType::TableHeader) {
                                QString cellText = extractTextContent(rowChild);
                                if(!cellText.isEmpty()) {
                                    cellContents.append("\\textbf{" + cellText + "}");
                                } else {
                                    cellContents.append("");
                                }
                            }
                        }
                        if(!cellContents.isEmpty()) {
                            latexTable += cellContents.join(" & ") + " \\\\\n\\hline\n";
                        }
                    }
                }
            } else if(child.blockType == MarkdownBlockType::TableBody) {
                //process data rows
                for(const auto& bodyChild : child.children) {
                    if(bodyChild.type == TextSegmentType::MarkdownBlock &&
                       bodyChild.blockType == MarkdownBlockType::TableRow) {
                        QStringList cellContents;
                        for(const auto& rowChild : bodyChild.children) {
                            if(rowChild.type == TextSegmentType::MarkdownBlock &&
                               rowChild.blockType == MarkdownBlockType::TableData) {
                                QString cellText = extractTextContent(rowChild);
                                if(!cellText.isEmpty()) {
                                    cellContents.append("\\text{" + cellText + "}");
                                } else {
                                    cellContents.append("");
                                }
                            }
                        }
                        if(!cellContents.isEmpty()) {
                            latexTable += cellContents.join(" & ") + " \\\\\n";
                        }
                    }
                }
            }
        }
    }

    latexTable += "\\hline\n\\end{array}";

    //render the LaTeX table as a display block
    tex::TeXRender* tableRender = parseDisplayLatexExpression(latexTable);
    if(tableRender != nullptr) {
        qreal renderWidth = tableRender->getWidth();
        qreal renderHeight = tableRender->getHeight();

        //center the table horizontally
        qreal tableX = maxWidth / 2 - renderWidth / 2;
        if(tableX < 5.0) tableX = 5.0; //minimum left margin

        //add some vertical spacing before the table
        y += lineHeight;

        //draw the LaTeX table
        qreal tableY = y - (renderHeight - tableRender->getDepth());
        tex::Graphics2D_qt g2(&painter);
        tableRender->draw(g2, static_cast<int>(tableX), static_cast<int>(tableY));

        //update position after table
        x = 5.0; //reset to left margin
        y += renderHeight + lineHeight; //add spacing after table

        delete tableRender;
    } else {
        //fallback to simple rendering if LaTeX parsing fails
        for(const auto& child : segment.children) {
            if(child.type == TextSegmentType::MarkdownBlock) {
                renderBlockElement(painter, child, x, y, maxWidth, lineHeight);
            }
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
    widget_height=y+50.0; // Add some padding at the bottom
    setMinimumHeight(widget_height);
    updateGeometry();
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
    qDebug() << "=== LIST NESTING DEBUG: After parsing ===";
    printSegmentsStructure();
    update();
    adjustSize();
}

void LatexLabel::printSegmentsStructure() const {
    qDebug() << "=== m_segments Structure ===";
    for(size_t i = 0; i < m_segments.size(); i++) {
        qDebug() << QString("m_segments[%1]:").arg(i);
        printSegmentRecursive(m_segments[i], 0);
    }
    qDebug() << "=== End Structure ===";
}

void LatexLabel::printSegmentRecursive(const TextSegment& segment, int depth) const {
    QString indent = QString("  ").repeated(depth);
    QString typeStr;

    switch(segment.type) {
        case TextSegmentType::MarkdownBlock:
            typeStr = "MarkdownBlock";
            break;
        case TextSegmentType::MarkdownSpan:
            typeStr = "MarkdownSpan";
            break;
        case TextSegmentType::MarkdownText:
            typeStr = "MarkdownText";
            break;
    }

    qDebug().noquote() << QString("%1TextSegment {").arg(indent);
    qDebug().noquote() << QString("%1  type: %2").arg(indent, typeStr);

    if(segment.type == TextSegmentType::MarkdownBlock) {
        QString blockTypeStr;
        switch(segment.blockType) {
            case MarkdownBlockType::Document: blockTypeStr = "Document"; break;
            case MarkdownBlockType::Quote: blockTypeStr = "Quote"; break;
            case MarkdownBlockType::UnorderedList: blockTypeStr = "UnorderedList"; break;
            case MarkdownBlockType::OrderedList: blockTypeStr = "OrderedList"; break;
            case MarkdownBlockType::ListItem: blockTypeStr = "ListItem"; break;
            case MarkdownBlockType::HorizontalRule: blockTypeStr = "HorizontalRule"; break;
            case MarkdownBlockType::Heading: blockTypeStr = "Heading"; break;
            case MarkdownBlockType::CodeBlock: blockTypeStr = "CodeBlock"; break;
            case MarkdownBlockType::HtmlBlock: blockTypeStr = "HtmlBlock"; break;
            case MarkdownBlockType::Paragraph: blockTypeStr = "Paragraph"; break;
            case MarkdownBlockType::Table: blockTypeStr = "Table"; break;
            case MarkdownBlockType::TableHead: blockTypeStr = "TableHead"; break;
            case MarkdownBlockType::TableBody: blockTypeStr = "TableBody"; break;
            case MarkdownBlockType::TableRow: blockTypeStr = "TableRow"; break;
            case MarkdownBlockType::TableHeader: blockTypeStr = "TableHeader"; break;
            case MarkdownBlockType::TableData: blockTypeStr = "TableData"; break;
        }
        qDebug().noquote() << QString("%1  blockType: %2").arg(indent, blockTypeStr);
        qDebug().noquote() << QString("%1  indentLevel: %2").arg(indent).arg(segment.indentLevel);

        if(segment.blockType == MarkdownBlockType::ListItem) {
            QString parentListStr = (segment.attributes.parentListType == MarkdownBlockType::OrderedList) ? "OrderedList" : "UnorderedList";
            qDebug().noquote() << QString("%1  attributes: {").arg(indent);
            qDebug().noquote() << QString("%1    parentListType: %2").arg(indent, parentListStr);
            if(segment.attributes.parentListType == MarkdownBlockType::OrderedList) {
                qDebug().noquote() << QString("%1    listItemNumber: %2").arg(indent).arg(segment.attributes.listItemNumber);
            }
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if(segment.blockType == MarkdownBlockType::Heading) {
            qDebug().noquote() << QString("%1  attributes: {").arg(indent);
            qDebug().noquote() << QString("%1    headingLevel: %2").arg(indent).arg(segment.attributes.headingLevel);
            qDebug().noquote() << QString("%1  }").arg(indent);
        }
    }

    if(segment.type == TextSegmentType::MarkdownSpan) {
        QString spanTypeStr;
        switch(segment.spanType) {
            case MarkdownSpanType::Emphasis: spanTypeStr = "Emphasis"; break;
            case MarkdownSpanType::Strong: spanTypeStr = "Strong"; break;
            case MarkdownSpanType::Link: spanTypeStr = "Link"; break;
            case MarkdownSpanType::Image: spanTypeStr = "Image"; break;
            case MarkdownSpanType::Code: spanTypeStr = "Code"; break;
            case MarkdownSpanType::Strikethrough: spanTypeStr = "Strikethrough"; break;
            case MarkdownSpanType::LatexMath: spanTypeStr = "LatexMath"; break;
            case MarkdownSpanType::LatexMathDisplay: spanTypeStr = "LatexMathDisplay"; break;
            case MarkdownSpanType::WikiLink: spanTypeStr = "WikiLink"; break;
            case MarkdownSpanType::Underline: spanTypeStr = "Underline"; break;
        }
        qDebug().noquote() << QString("%1  spanType: %2").arg(indent, spanTypeStr);
    }

    if(segment.type == TextSegmentType::MarkdownText) {
        QString textTypeStr;
        switch(segment.textType) {
            case MarkdownTextType::Normal: textTypeStr = "Normal"; break;
            case MarkdownTextType::NullChar: textTypeStr = "NullChar"; break;
            case MarkdownTextType::HardBreak: textTypeStr = "HardBreak"; break;
            case MarkdownTextType::SoftBreak: textTypeStr = "SoftBreak"; break;
            case MarkdownTextType::Entity: textTypeStr = "Entity"; break;
            case MarkdownTextType::Code: textTypeStr = "Code"; break;
            case MarkdownTextType::Html: textTypeStr = "Html"; break;
            case MarkdownTextType::LatexMath: textTypeStr = "LatexMath"; break;
        }
        qDebug().noquote() << QString("%1  textType: %2").arg(indent, textTypeStr);

        if(!segment.content.isEmpty()) {
            QString contentStr = segment.content;
            if(contentStr.length() > 50) {
                contentStr = contentStr.left(47) + "...";
            }
            contentStr = contentStr.replace('\n', "\\n").replace('\t', "\\t");
            qDebug().noquote() << QString("%1  content: \"%2\"").arg(indent, contentStr);
        }

        if(segment.render != nullptr) {
            qDebug().noquote() << QString("%1  render: (LaTeX render object - width: %2, height: %3)")
                .arg(indent)
                .arg(segment.render->getWidth())
                .arg(segment.render->getHeight());
        }
    }

    if(!segment.children.empty()) {
        qDebug().noquote() << QString("%1  children: [").arg(indent);
        for(size_t i = 0; i < segment.children.size(); i++) {
            if(i > 0) qDebug().noquote() << QString("%1    ,").arg(indent);
            printSegmentRecursive(segment.children[i], depth + 2);
        }
        qDebug().noquote() << QString("%1  ]").arg(indent);
    }

    qDebug().noquote() << QString("%1}").arg(indent);
}
