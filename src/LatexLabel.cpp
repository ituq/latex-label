#include "LatexLabel.h"
#include "platform/qt/graphic_qt.h"
#include "utils/enums.h"
#include <QRegularExpression>
#include <QFontMetrics>
#include <QSizePolicy>
#include <QDebug>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QTextOption>
#include <QFrame>
#include <QStyleOption>
#include <QStyle>
#include <QPainterPath>
#include <md4c.h>
#include <variant>
#include <vector>

static double widget_height=200;

LatexLabel::LatexLabel(QWidget* parent) : QWidget(parent), _render(nullptr), m_textSize(12) {

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    QFontMetricsF base_metric(QFont("Arial",m_textSize));
    setAttribute(Qt::WA_StyledBackground, true);
    widget_height=300; // Start with a reasonable height
    setMinimumHeight(widget_height);
}

LatexLabel::~LatexLabel(){
    //Clean up all segments and their children
    clear_code_block_widgets();
    cleanup_segments(m_segments);
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

tex::TeXRender* getLatexRenderer(const QString& latex, bool isInline, int text_size, QRgb argb_color) {
    try {
        tex::Formula formula;
        formula.setLaTeX(latex.toStdWString());
        float width = 600;
        tex::Alignment alignment= isInline ? tex::Alignment::left : tex::Alignment::center;
        float linespace = isInline ? text_size : text_size + 2;
        tex::TexStyle style = isInline ? tex::TexStyle::text : tex::TexStyle::display;

        tex::TeXRenderBuilder builder;
        tex::TeXRender* render = builder
            .setStyle(style)
            .setTextSize(text_size)
            .setWidth(tex::UnitType::pixel, width, alignment)
            .setIsMaxWidth(true)
            .setLineSpace(tex::UnitType::point, linespace)
            .setForeground(argb_color)
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
    Element* block = new Element(DisplayType::block, {}, spantype::normal, type);
    switch(type) {
        case MD_BLOCK_DOC:{
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_QUOTE:{
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_UL:{
            list_data data;
            data.is_ordered = false;
            if(detail){
                MD_BLOCK_UL_DETAIL* ul_detail = (MD_BLOCK_UL_DETAIL*) detail;

                data.mark=ul_detail->mark;
            }
            block->data=data;
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_OL:{
            list_data data;
            data.is_ordered = true;
            if(detail) {
                MD_BLOCK_OL_DETAIL* ol_detail = (MD_BLOCK_OL_DETAIL*) detail;
                data.start_index = (uint16_t) ol_detail->start;
                data.mark= ol_detail->mark_delimiter;
            }
            block->data=data;
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_LI:{
            list_item_data data;
            Element* parent = state->blockStack.back();
            data.is_ordered = std::get<list_data>(parent->data).is_ordered;
            data.item_index = parent->children.size()+1;
            block->data=data;
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);

            break;
        }
        case MD_BLOCK_HR:{
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_H:{
            heading_data data;
            if(detail) {
                MD_BLOCK_H_DETAIL* h_detail = (MD_BLOCK_H_DETAIL*) detail;
                data.level = h_detail->level;
            }
            else{
                data.level = 1;
            }
            block->data=data;
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_CODE:{
            code_block_data data;
            if(detail) {
                MD_BLOCK_CODE_DETAIL* code_detail = (MD_BLOCK_CODE_DETAIL*)(detail);
                if(code_detail->lang.text) {
                    data.language = QString::fromUtf8(code_detail->lang.text, code_detail->lang.size);
                }
            }
            block->data=data;
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_HTML:{
            qDebug()<<"html block not supported";
            break;
        }
        case MD_BLOCK_P:{
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_TABLE:{
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_THEAD:{
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);

            break;
        }
        case MD_BLOCK_TBODY:{
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_TR: {
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_TH: {
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_TD: {
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        default:
            break;
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

    if( type==MD_BLOCK_DOC){
        state->segments=state->blockStack.back()->children;
    }
    else if(type == MD_BLOCK_CODE){
        if(!state->blockStack.back()->children.empty()) {
            state->blockStack.back()->children.pop_back(); // remove final \n
        }
    }

    state->blockStack.pop_back();


    return 0;
}

int LatexLabel::enterSpanCallback(MD_SPANTYPE type, void* detail, void* userdata) {
    struct ExtendedParserState {
        MarkdownParserState* state;
        LatexLabel* label;
    };
    ExtendedParserState* extState = static_cast<ExtendedParserState*>(userdata);
    MarkdownParserState* state = extState->state;
    spantype span_type = spantype::normal;

    switch(type) {
        case MD_SPAN_EM:
            span_type = spantype::italic;
            break;
        case MD_SPAN_STRONG:
            if(!state->spanStack.empty()&&SPANTYPE(state->spanStack.back())==spantype::italic){
                //replace both with italic_bold span
                state->blockStack.back()->children.pop_back();
                Element* ital = state->spanStack.back();
                state->spanStack.pop_back();
                delete ital;
                span_type = spantype::italic_bold;
                break;
            }
            span_type = spantype::bold;
            break;
        case MD_SPAN_A:{
            link_data data;

            if(detail) {
                MD_SPAN_A_DETAIL* a_detail = (MD_SPAN_A_DETAIL*)detail;
                if(a_detail->href.text) {
                    data.url = QString::fromUtf8(a_detail->href.text, a_detail->href.size);
                }
                if(a_detail->title.text) {
                    data.title = QString::fromUtf8(a_detail->title.text, a_detail->title.size);
                }
            }
            else{
                data.url = "Error parsing link";
                data.title = "Error parsing link";
            }
            Element* span = new Element(DisplayType::span, data, spantype::hyperlink);
            state->blockStack.back()->children.push_back(span);
            state->spanStack.push_back(span);
            return 0;
        }
        case MD_SPAN_IMG:
            span_type = spantype::image;
            qDebug()<<"images are not supported yet";
            break;
        case MD_SPAN_CODE:
            span_type = spantype::code;
            break;
        case MD_SPAN_DEL:
            span_type = spantype::strikethrough;
            break;
        case MD_SPAN_LATEXMATH:{
            latex_data data;
            data.isInline=true;
            data.render = nullptr;
            data.text = "";
            Element* span = new Element(DisplayType::span, data, spantype::latex);
            state->blockStack.back()->children.push_back(span);
            state->spanStack.push_back(span);
            return 0;
        }
        case MD_SPAN_LATEXMATH_DISPLAY:{
            latex_data data;
            data.isInline=false;
            data.render = nullptr;
            data.text = "";
            Element* span = new Element(DisplayType::span, data, spantype::latex);
            state->blockStack.back()->children.push_back(span);
            state->spanStack.push_back(span);
            return 0;
        }
        case MD_SPAN_WIKILINK:
            qDebug()<<"wiki links are not supported yet";
            break;
        case MD_SPAN_U:
            span_type = spantype::underline;
            break;
    }

    span_data data;
    data.text = "";
    Element* span = new Element(DisplayType::span, data, span_type);
    state->blockStack.back()->children.push_back(span);
    state->spanStack.push_back(span);

    return 0;
}

int LatexLabel::leaveSpanCallback(MD_SPANTYPE type, void* detail, void* userdata) {
    struct ExtendedParserState {
        MarkdownParserState* state;
        LatexLabel* label;
    };
    ExtendedParserState* extState = static_cast<ExtendedParserState*>(userdata);
    MarkdownParserState* state = extState->state;
    switch(type) { //some spans don't add to span stack
        case MD_SPAN_IMG:
            return 0;
        case MD_SPAN_WIKILINK:
            return 0;
        case MD_SPAN_STRONG:
            if(SPANTYPE(state->spanStack.back())==spantype::italic_bold){
                return 0;
            }
        default:
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
    label->m_raw_text+=textStr;
    if(state->spanStack.empty()){ //no open span, add to recent block element
        Element* block= state->blockStack.back();

        span_data data;
        switch(type) {
            case MD_TEXT_NORMAL:{
                data.text = textStr;
                Element* t = new Element(DisplayType::span,data,spantype::normal);
                block->children.push_back(t);

            }
                break;
            case MD_TEXT_NULLCHAR:{

                data.text = QChar(0xFFFD); // Unicode replacement character
                Element* t = new Element(DisplayType::span,data,spantype::normal);
                block->children.push_back(t);
            }
                break;
            case MD_TEXT_BR:{

                Element* t = new Element(DisplayType::span,data,spantype::linebreak);
                block->children.push_back(t);
            }
                break;
            case MD_TEXT_SOFTBR:
                return 0; // ignore soft break
                break;
            case MD_TEXT_ENTITY:
                qDebug()<<"entity is not supported yet";
                return 0; // ignore entity
                break;
            case MD_TEXT_CODE:{
                data.text = textStr;
                Element* t = new Element(DisplayType::span,data,spantype::code);
                block->children.push_back(t);
            }

                break;
            case MD_TEXT_HTML:
            qDebug()<<"html is not supported yet";
                return 0;
                break;
            case MD_TEXT_LATEXMATH: // this shouldn't be possible
                qDebug()<<"latex text shouldn't be here";
                return 0;
                break;
        }
        return 0;
    }

    //we have an open span, fill it with text

    Element* parent_span = state->spanStack.back();
    ElementData* data_parent=&parent_span->data;

    switch(type) {
        case MD_TEXT_NORMAL:
            if(SPANTYPE(parent_span)==spantype::hyperlink){
                link_data* data=std::get_if<link_data>(data_parent);
                data->title=textStr;
                break;
            }
            {
                span_data* span_data_ptr = std::get_if<span_data>(data_parent);
                if(span_data_ptr) {
                    span_data_ptr->text = textStr;
                }
            }
            break;
        case MD_TEXT_NULLCHAR:
            {
                span_data* span_data_ptr = std::get_if<span_data>(data_parent);
                if(span_data_ptr) {
                    span_data_ptr->text = QChar(0xFFFD); // Unicode replacement character
                }
            }
            break;
        case MD_TEXT_BR: //we change the span type to a linebreak
            SPANTYPE(parent_span)=spantype::linebreak;
            break;
        case MD_TEXT_SOFTBR:// do nothing
            break;
        case MD_TEXT_ENTITY: //do nothing
            break;
        case MD_TEXT_CODE:
            {
                span_data* span_data_ptr = std::get_if<span_data>(data_parent);
                if(span_data_ptr) {
                    span_data_ptr->text = textStr;
                }
            }
            break;
        case MD_TEXT_HTML://do nothing
            break;
        case MD_TEXT_LATEXMATH:
            {
                latex_data* data_latex = std::get_if<latex_data>(data_parent);
                if(data_latex) {
                    QRgb argb_color = label->palette().text().color().rgba();
                    data_latex->render=getLatexRenderer(textStr, data_latex->isInline, state->textSize, argb_color);
                    data_latex->text=textStr;
                }
            }
            break;
    }


    return 0;
}

void LatexLabel::cleanup_segments(std::vector<Element*>& segments) {
    for(Element* elem : segments) {
        if(elem == nullptr) continue;
        delete elem;
    }
    m_segments.clear();
}

void LatexLabel::parseMarkdown(const QString& text) {
    //Clean up previous segments
    clear_code_block_widgets();
    cleanup_segments(m_segments);

    // Set up parser state
    MarkdownParserState state(m_textSize);


    // Store a reference to this LatexLabel instance in the state
    struct ExtendedParserState {
        MarkdownParserState* state;
        LatexLabel* label;
    };
    ExtendedParserState extendedState = { &state, this };

    // Set up md4c parser
    MD_PARSER parser = {0};
    parser.abi_version = 0;
    parser.flags = MD_FLAG_LATEXMATHSPANS | MD_FLAG_STRIKETHROUGH | MD_FLAG_TABLES;
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

        //populate m_display_list
        QFontMetricsF fontMetrics = QFontMetricsF(getFont(font_type::normal));
        qreal lineHeight = fontMetrics.lineSpacing();
        qreal x = margin_left;
        qreal y = margin_top + fontMetrics.ascent();  // y represents the text baseline
        qreal maxWidth = width() - 10.0;

        for(const Element* segment : m_segments) {
            if(segment->type==DisplayType::block){
                renderBlock(*segment, x, y,5.0,width(), lineHeight);
            }
            else{
                renderSpan(*segment, x, y,5.0,width(), lineHeight);
            }

        }
        widget_height=y;
        setMinimumHeight(widget_height);
        updateGeometry();
    } else {
        //Clean up any partial parsing results
        cleanup_segments(state.segments);
        qDebug() << "Markdown parsing failed, result code:" << result;
    }
}

QFont LatexLabel::getFont(font_type type) const {
    // Start with the base font
    QFont font("Arial", m_textSize);

    switch (type) {
        case font_type::bold:
            font.setBold(true);
            break;

        case font_type::italic:
            font.setItalic(true);
            break;

        case font_type::italic_bold:
            font.setBold(true);
            font.setItalic(true);
            break;

        case font_type::mono:
            font.setFamily("Monaco");
            break;
        case font_type::strikethrough:
                    font.setStrikeOut(true);
                    break;

        case font_type::underline:
        case font_type::hyperlink: // Links are also underlined
            font.setUnderline(true);
            break;

        // Handle all heading levels
        case font_type::heading1:
        case font_type::heading2:
        case font_type::heading3:
        case font_type::heading4:
        case font_type::heading5:
        case font_type::heading6:
        {
            int level = 0;
            if (type == font_type::heading1) level = 1;
            else if (type == font_type::heading2) level = 2;
            else if (type == font_type::heading3) level = 3;
            else if (type == font_type::heading4) level = 4;
            else if (type == font_type::heading5) level = 5;
            else if (type == font_type::heading6) level = 6;
            // Original formula: textSize + (6 - level) * 4
            int headingSize = std::max(8, m_textSize + (6 - level) * 4);
            font.setPointSize(headingSize);
            font.setBold(true);
            break;
        }

        case font_type::normal:
        default:
            // Do nothing, use the default font
            break;
    }

    return font;
}
QFont LatexLabel::getFont(const Element* segment) const {
    QFont font("Arial", m_textSize);

    if(segment->type==DisplayType::block&&BLOCKTYPE(segment)==MD_BLOCK_H){
        heading_data data = std::get<heading_data>(segment->data);
        int headingSize = std::max(8, m_textSize + (6 - data.level) * 4);
        font.setPointSize(headingSize);
        font.setBold(true);
    }
    else if (segment->type==DisplayType::block&&BLOCKTYPE(segment)==MD_BLOCK_CODE){
        font.setFamily("Monaco");
    }
    else if(segment->type==DisplayType::span){

        switch (SPANTYPE(segment)) {
            case spantype::bold:
                font.setBold(true);
                break;
            case spantype::italic:
                font.setItalic(true);
                break;
            case spantype::underline:
                font.setUnderline(true);
                break;
            case spantype::hyperlink:
                font.setUnderline(true);
                break;
            case spantype::strikethrough:
                font.setStrikeOut(true);
                break;
            case spantype::code:
                font.setFamily("Monaco");
                break;
            case spantype::italic_bold:
                font.setBold(true);
                font.setItalic(true);
                break;
            default:
                break;
        }
    }

    return font;
}



qreal LatexLabel::getLineHeight(const Element& segment, const QFontMetricsF& metrics) const {
    if (segment.type==DisplayType::block&& *((MD_BLOCKTYPE*)segment.subtype)==MD_BLOCK_H) {
        return metrics.height() * 1.2; // Extra spacing for headings
    } else if (segment.type==DisplayType::block&& *((MD_BLOCKTYPE*)segment.subtype)==MD_BLOCK_CODE) {
        return metrics.height() * 1.1; // Slight extra spacing for code blocks
    }

    return metrics.height();
}

void LatexLabel::renderSpan(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal lineHeight,QFont* font_passed) {
    QFont font;
    if(font_passed){
        font=*font_passed;
    }
    else{
        font = getFont(&segment);
    }
    spantype type = *(spantype*)segment.subtype;
    QColor color = type==spantype::hyperlink ? Qt::blue : Qt::black;



    QFontMetrics metrics(font);



    // Handle different text types

    if(type == spantype::linebreak) {
        y += metrics.lineSpacing();
        return;
    }


    // Handle LaTeX math
    if(type == spantype::latex) {
        latex_data data = std::get<latex_data>(segment.data);
        qreal renderWidth = data.render->getWidth();
        qreal renderHeight = data.render->getHeight();

        if(data.isInline&& x+renderWidth>max_x){
            //inline latex, check if it fits on line
            x = min_x;
            y += metrics.lineSpacing();

        }
        else if(!data.isInline){
            y += 2*metrics.height();
            x=(max_x-min_x)/2-renderWidth/2;
        }

        // Draw LaTeX expression
        qreal latexY = y - (renderHeight - data.render->getDepth());
        int width=data.render->getWidth();
        int height= data.render->getHeight();
        addLatex(x, latexY, width, height, data.render, data.text, data.isInline);



        if(data.isInline){
            x += renderWidth + metrics.horizontalAdvance(" ");
        }
        else{
            x = min_x;
            y += (renderHeight/2)+metrics.height();
        }

        return;
    }


    QString text;
    if(type!=spantype::hyperlink){
        text = std::get<span_data>(segment.data).text;
    }
    else{
        link_data link_infos= std::get<link_data>(segment.data);
        text=link_infos.url;
    }




    QStringList words = text.split(' ', Qt::SkipEmptyParts);

    for(const QString& word : words) {
        if(word=="\n"){
            x = min_x;
            y += metrics.lineSpacing();
            continue;
        }
        //Calculate word width
        int wordWidth = metrics.horizontalAdvance(word);
        int spaceWidth = metrics.horizontalAdvance(" ");
        int totalWidth = wordWidth + spaceWidth;

        //Check if word fits on current line
        if(x + totalWidth > max_x) {
            x = min_x;
            y += metrics.lineSpacing();
        }
        addText(x, y-metrics.ascent(), wordWidth+1, metrics.height(), word, font);
        x += totalWidth;
    }
}


void LatexLabel::renderBlock(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight){
    QFont font = getFont(&segment);
    QFontMetricsF metrics(font);
    qreal currentLineHeight = getLineHeight(segment, metrics);

    switch(*(MD_BLOCKTYPE*)segment.subtype) {
        case MD_BLOCK_DOC:
            // Document block - render all children
            for(const Element* child : segment.children) {
                if(child->type==DisplayType::block) {
                    renderBlock(*child, x, y, min_x,max_x, lineHeight);
                } else {
                    renderSpan(*child, x, y, min_x,max_x, lineHeight);
                }
            }
            break;
        case MD_BLOCK_H:
            renderHeading(segment, x, y, min_x,max_x, lineHeight);
            break;
        case MD_BLOCK_CODE:
            renderCodeBlock(segment, x, y, min_x,max_x, lineHeight);
            break;
        case MD_BLOCK_QUOTE:
            renderBlockquote(segment, x, y, min_x,max_x, lineHeight);
            break;
        case MD_BLOCK_UL:
        case MD_BLOCK_OL:
        case MD_BLOCK_LI:
            renderListElement(segment, x, y, min_x,max_x, lineHeight);
            break;
        case MD_BLOCK_TABLE:
            renderTable(segment, x, y, min_x, max_x, lineHeight);
            break;
        case MD_BLOCK_HR:{
            // Draw horizontal line
            y += 2*lineHeight;
            addLine(x, y, width()-5, y, QPoint(width()-5, y));
            y += 2*lineHeight;
            break;
        }
        case MD_BLOCK_P:
            {
                QFont baseFont("Arial", m_textSize);
                for(const Element* child : segment.children) {
                        renderSpan(*child, x, y,min_x,max_x, lineHeight);
                }
                x = min_x;
                y += currentLineHeight;
            }
            break;
        default:
            // Default block rendering
            {
                qDebug()<<"using default block rendering";
                for(const auto& child : segment.children) {
                    renderSpan(*child, x, y,min_x,max_x, lineHeight);
                }
            }
            break;
    }
}

void LatexLabel::renderListElement(const Element& segment, qreal& x, qreal& y, int min_x,int max_x, qreal& lineHeight) {

    MD_BLOCKTYPE type =*(MD_BLOCKTYPE*)segment.subtype;


    if(type==MD_BLOCK_LI) {
        list_item_data data =std::get<list_item_data>(segment.data);
        //draw list marker
        QFont baseFont("Arial", m_textSize);



        QString marker;
        if(data.is_ordered) {
            //ordered list - use item number
            marker = QString::number(data.item_index) + ". ";
        } else {
            //unordered list - use bullet
            marker = "• "; //TODO: replace with actual markers
        }
        //adjust x position based on marker width
        QFontMetrics metrics(baseFont);
        qreal markerWidth = metrics.horizontalAdvance(marker);
        addText(x, y-metrics.ascent(), markerWidth, metrics.height(), marker, baseFont);


        x += markerWidth + 2.0;


        qreal left_border = x;



        // Render list item content
        QFont listFont("Arial", m_textSize);
        bool last_rendered_is_list=false;
        for(const Element* child : segment.children) {
            last_rendered_is_list=false;
            if(child->type==DisplayType::span) {
                renderSpan(*child, x, y, left_border,max_x, lineHeight);
            }
            else{
                last_rendered_is_list=BLOCKTYPE(child)==MD_BLOCK_UL||BLOCKTYPE(child)==MD_BLOCK_OL;
                if(x>left_border){
                    x=left_border;
                    y+=lineHeight;
                }
                renderBlock(*child, x, y, left_border,max_x, lineHeight);
            }
        }
        if(!last_rendered_is_list){
            //don't add padding after list if we're not in nested list
            y += lineHeight * 1.1;
        }
        x = min_x;
    } else {
        // List container - render children
        for(const Element* child : segment.children) {
            if(child->type==DisplayType::block) {
                renderBlock(*child, x, y, min_x,max_x, lineHeight);
            }
        }
        x=min_x;
    }
}

void LatexLabel::renderHeading(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight) {

    QFont font = getFont(&segment);
    QFontMetricsF metrics(font);

    qreal headingLineHeight = metrics.lineSpacing();

    // Add more spacing before heading (move to next line if not at start)
    if(x > min_x) {
        x = min_x;
        y += lineHeight;
    }
    y += headingLineHeight * 0.8; // Increased spacing before heading


    // Render heading content with heading font inherited
    for(const Element* child : segment.children) {
        renderSpan(*child, x, y, min_x,max_x, headingLineHeight,&font);
    }

    // Add spacing after heading and move to next line
    x = min_x;
    y += headingLineHeight * 0.8; // Increased spacing after heading

}

void LatexLabel::renderCodeBlock(const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight) {
    x+=15;
    y+=15;
    //Set code font and assemble full text
    QFont font = getFont(&segment);
    font.setPointSize(m_textSize);
    QFontMetrics fm(font);

    int header_padding=4;
    int headerHeight = fm.height()+2*header_padding;


    unsigned lineCount =0;
    for(const Element* child :segment.children){
        if(std::get<span_data> (child->data).text=="\n")
            lineCount++;
    }
    int content_height=fm.ascent()+fm.lineSpacing()*lineCount+headerHeight;
    QRect background(x,y-fm.ascent()-5,max_x-2*x,content_height+fm.descent()+10);
    addRoundedRect(x, y-fm.ascent()-5, max_x-2*x, content_height+fm.descent()+10, background, 10, QPalette::ColorRole::Base);

    QRect header_background(x,y-fm.ascent()-5,max_x-2*x,headerHeight);
    addRoundedRect(x, y-fm.ascent()-5, max_x-2*x, headerHeight, header_background, 10, 10, 0, 0, QPalette::ColorRole::WindowText);



    y+=headerHeight;

    x+=10;



    for(const Element* child : segment.children) {
        //high max_x to prevent wrapping
        renderSpan(*child, x, y, min_x+10+15, 10000, lineHeight);
    }

    code_block_data data = std::get<code_block_data>(segment.data);
    QString language;

    language = data.language;

    //Create embedded code widget with horizontal scrolling and copy button
    //create_code_block_widget(block_rect, full_text, font, language);

    //Advance layout positions
    y+=fm.lineSpacing();
    y+=10;
    x = min_x;
}

void LatexLabel::renderBlockquote(const Element& segment, qreal& x, qreal& y, qreal min_x, qreal max_x, qreal& lineHeight) {
    // Add spacing before blockquote and move to next line if not at start
    y += lineHeight * 0.5; // Add spacing before blockquote

    // Indent content
    x += 50;
    qreal min_x_children = x;

    // Record starting position for border
    qreal startY = y;
    qreal startX = x;

    // Render blockquote content
    QFont blockquoteFont("Arial", m_textSize);
    QFontMetricsF metrics(blockquoteFont);
    for(const Element* child : segment.children) {
        if(child->type==DisplayType::block) {
            renderBlock(*child, x, y, min_x+50,max_x, lineHeight);
        } else {
            renderSpan(*child, x, y, min_x+50, max_x, lineHeight, &blockquoteFont);
        }
    }

    //draw line left of quote
    QPoint endpoint=QPoint(startX-5, y-metrics.height() + metrics.descent());
    addLine(startX-5, startY - metrics.ascent(), 3, y, endpoint, 3);

    // Reset position and add spacing after blockquote
    x = min_x;
    y += lineHeight * 1.2; // Add more spacing after blockquote


}
QString column_string(int n){
    QString res="|";
    for (int i=0; i<n; i++) {
        res+="c|";
    }
    return res;
}

void LatexLabel::calculate_table_dimensions(const Element& segment, int* max_width_of_col, int* max_height_of_row, int columns, int rows, qreal min_x, qreal max_x) {
    //Calculate equal column widths to fill available space
    int padding = 5;
    qreal available_width = max_x - min_x - 10; //subtract margin
    qreal column_width = (available_width - (columns + 1) * 2 * padding) / columns; //subtract padding for all columns

    //Set all columns to equal width
    for(int i = 0; i < columns; i++) {
        max_width_of_col[i] = (int)column_width;
    }

    //Initialize row heights to zero
    for(int i = 0; i < rows; i++) {
        max_height_of_row[i] = 0;
    }

    QFont base_font("Arial", m_textSize);
    QFontMetrics base_metrics(base_font);

    int current_row = 0;

    //Process all children (TableHead and TableBody) to calculate row heights
    for(const Element* table_section : segment.children) {
        if(table_section->type != DisplayType::block) continue;

        MD_BLOCKTYPE section_type = BLOCKTYPE(table_section);

        //Process each row in this section
        for(const Element* row : table_section->children) {
            if(row->type != DisplayType::block || BLOCKTYPE(row) != MD_BLOCK_TR) continue;

            int current_col = 0;
            int row_max_height = 0;

            //Process each cell in this row
            for(const Element* cell : row->children) {
                if(cell->type != DisplayType::block) continue;
                if(BLOCKTYPE(cell) != MD_BLOCK_TH && BLOCKTYPE(cell) != MD_BLOCK_TD) continue;

                //Compute height by simulating wrapping across all spans in order
                int available_width = max_width_of_col[current_col] + padding;
                if(available_width <= 0) available_width = 100;

                int current_x_sim = 0;
                int current_line_height = 0;
                int cell_height = 0;

                for(const Element* content : cell->children) {
                    if(content->type != DisplayType::span) continue;

                    spantype span_type = SPANTYPE(content);

                    if(span_type == spantype::latex) {
                        latex_data latex_details = std::get<latex_data>(content->data);
                        int latex_width = (int)latex_details.render->getWidth();
                        int latex_height = (int)latex_details.render->getHeight();

                        //Use base font for space width when advancing after inline latex
                        QFont base_font("Arial", m_textSize);
                        QFontMetrics base_metrics(base_font);
                        int space_width = base_metrics.horizontalAdvance(" ");

                        if(current_x_sim + latex_width > available_width) {
                            cell_height += current_line_height > 0 ? current_line_height : base_metrics.lineSpacing();
                            current_x_sim = 0;
                            current_line_height = 0;
                        }

                        current_x_sim += latex_width + space_width;
                        if(latex_height > current_line_height) current_line_height = latex_height;
                        continue;
                    }

                    if(span_type == spantype::normal || span_type == spantype::code ||
                       span_type == spantype::bold || span_type == spantype::italic ||
                       span_type == spantype::italic_bold || span_type == spantype::underline ||
                       span_type == spantype::hyperlink || span_type == spantype::strikethrough) {
                        span_data text_data = std::get<span_data>(content->data);

                        QFont font = getFont(content);
                        QFontMetrics metrics(font);
                        QStringList words = text_data.text.split(' ', Qt::SkipEmptyParts);

                        for(const QString& word : words) {
                            if(word == "\n") {
                                cell_height += current_line_height > 0 ? current_line_height : metrics.lineSpacing();
                                current_x_sim = 0;
                                current_line_height = 0;
                                continue;
                            }

                            int word_width = metrics.horizontalAdvance(word);
                            int space_width = metrics.horizontalAdvance(" ");
                            int total_width = word_width + space_width;

                            if(current_x_sim + total_width > available_width) {
                                cell_height += current_line_height > 0 ? current_line_height : metrics.lineSpacing();
                                current_x_sim = 0;
                                current_line_height = 0;
                            }

                            current_x_sim += total_width;
                            if(metrics.lineSpacing() > current_line_height) current_line_height = metrics.lineSpacing();
                        }
                    }
                }

                if(current_line_height > 0) {
                    cell_height += current_line_height;
                }

                //Track row max height
                row_max_height = std::max(row_max_height, cell_height);

                current_col++;
            }

            //Update row max height
            if(current_row < rows) {
                max_height_of_row[current_row] = row_max_height;
            }

            current_row++;
        }
    }
}

void LatexLabel::renderTable(const Element& segment, qreal& x, qreal& y, qreal min_x, qreal max_x, qreal& lineHeight) {
    y+=10;
    QFontMetrics fm(getFont(&segment));
    bool header_only=segment.children.size()==1;

    Element* table_header_row=segment.children[0]->children[0];
    Element* table_body = segment.children[1];

    int columns=table_header_row->children.size();
    int rows = 1;
    if(!header_only){
        rows+=table_body->children.size();//table head + children of table body
    }

    int max_width_of_col[columns];
    int max_height_of_row[rows];
    int padding =5; //to all sides

    //Calculate table dimensions
    calculate_table_dimensions(segment, max_width_of_col, max_height_of_row, columns, rows, min_x, max_x);
    //combine table header and table body into one list
    std::vector<Element*> row_list;
    if(header_only){
        row_list.push_back(table_header_row);
    }
    else{
        row_list=table_body->children;
        row_list.insert(row_list.begin(),table_header_row);
    }

    //render table
    for(int i=0;i<row_list.size();i++){
        Element* row = row_list[i];
        for (int j=0;j<row->children.size();j++) {
            Element* cell =row->children[j];

            QRect cell_border =QRect(x,y-fm.ascent()-padding,max_width_of_col[j]+2*padding,max_height_of_row[i]+2*padding);
            addRoundedRect(x, y-fm.ascent()-padding, max_width_of_col[j]+2*padding, max_height_of_row[i]+2*padding, cell_border, 0, QPalette::ColorRole::Window);
            int cell_start_x=x;
            int cell_start_y=y;
            x+=padding;


            for (Element* e : cell->children) {
                if(e->type==DisplayType::span){
                    renderSpan(*e, x, y, cell_start_x+padding, cell_start_x+max_width_of_col[j]+2*padding, lineHeight);
                }
            }
            x=cell_start_x+max_width_of_col[j]+2*padding;
            y=cell_start_y;
        }
        y+=max_height_of_row[i]+2*padding;
        x=min_x;
    }

}

void LatexLabel::paintEvent(QPaintEvent* event){
    QRect area=event->rect();

    /*
    //timing variables
    clock_t start = clock();


    static double total_time = 0.0;
    static int iteration_count = 0;
    static const int max_iterations = 50; // Reset average every 50 paint events
     */

    QPainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::black);



    for(Fragment& f: m_display_list){
        if(!area.intersects(f.bounding_box)) continue;
        if(f.is_highlighted){
            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(palette().highlight());
            painter.drawRect(f.bounding_box);
            painter.restore();
            painter.setPen(palette().highlightedText().color());
        }
        switch (f.type) {
            case fragment_type::latex:{
                frag_latex_data* data = (frag_latex_data*) f.data;
                painter.save();
                painter.setBrush(palette().text());

                tex::Graphics2D_qt g2(&painter);
                data->render->draw(g2, f.bounding_box.x(), f.bounding_box.y());
                painter.restore();
                break;
            }
            case fragment_type::line:{
                painter.save();
                frag_line_data* data = (frag_line_data*) f.data;
                painter.setPen(QPen(palette().mid(), data->width));

                painter.drawLine(QPoint(f.bounding_box.x(),f.bounding_box.y()),data->to);
                painter.restore();
                break;
            }
            case fragment_type::rounded_rect:{
                frag_rrect_data* data = (frag_rrect_data*) f.data;
                QBrush backgroundBrush = palette().brush(data->background);
                painter.setBrush(backgroundBrush);
                painter.setPen(palette().windowText().color());

                // Create custom rounded rectangle with individual corner radii
                QPainterPath path;
                QRectF rect = data->rect;
                qreal x = rect.x();
                qreal y = rect.y();
                qreal w = rect.width();
                qreal h = rect.height();

                qreal tl = qMin(data->topLeftRadius, qMin(w/2, h/2));
                qreal tr = qMin(data->topRightRadius, qMin(w/2, h/2));
                qreal bl = qMin(data->bottomLeftRadius, qMin(w/2, h/2));
                qreal br = qMin(data->bottomRightRadius, qMin(w/2, h/2));

                path.moveTo(x + tl, y);
                path.lineTo(x + w - tr, y);
                if (tr > 0) path.arcTo(x + w - 2*tr, y, 2*tr, 2*tr, 90, -90);
                path.lineTo(x + w, y + h - br);
                if (br > 0) path.arcTo(x + w - 2*br, y + h - 2*br, 2*br, 2*br, 0, -90);
                path.lineTo(x + bl, y + h);
                if (bl > 0) path.arcTo(x, y + h - 2*bl, 2*bl, 2*bl, 270, -90);
                path.lineTo(x, y + tl);
                if (tl > 0) path.arcTo(x, y, 2*tl, 2*tl, 180, -90);
                path.closeSubpath();

                painter.drawPath(path);
                break;
            }
            case fragment_type::text:{
                frag_text_data* data = (frag_text_data*) f.data;
                painter.setFont(data->font);
                painter.setPen(palette().text().color());
                painter.drawText(f.bounding_box,data->text);
                break;
            }
        }
        if(f.is_highlighted){
            painter.setPen(Qt::black);
        }
    }
    /*
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    //Update running average
    total_time += elapsed;
    iteration_count++;

    if(iteration_count >= max_iterations) {
        double average_time = total_time / iteration_count;
        qDebug() << QString("Average rendering time over %1 iterations: %2 ms")
                    .arg(iteration_count)
                    .arg(average_time * 1000, 0, 'f', 3);

        //Reset for next averaging period
        total_time = 0.0;
        iteration_count = 0;
    }

     */

}

void LatexLabel::changeEvent(QEvent* event) {
    if(event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        m_pallete = palette();

        //Update color for all latex fragments
        QRgb argb_color = palette().text().color().rgba();
        for(Fragment& f : m_display_list){
            if(f.type != fragment_type::latex) continue;
            frag_latex_data* data = (frag_latex_data*) f.data;
            if(data && data->render){
                //Update foreground color of the existing renderer
                data->render->setForeground(static_cast<tex::color>(argb_color));
            }
        }

        update();
    }
    QWidget::changeEvent(event);
}

void LatexLabel::resizeEvent(QResizeEvent* event) {

    QWidget::resizeEvent(event);

    if(!m_text.isEmpty() && event->oldSize().width() != event->size().width()) {
        deleteDisplayList();
        clear_code_block_widgets();
        parseMarkdown(m_text);//this is expensive, need to find way to avoid without dangling latex pointers in m_segments
        update();
    }
}


void LatexLabel::appendText(QString& text){
    if(text.isEmpty()) return;
    m_text += text;

    //clock_t start = clock();
    deleteDisplayList();
    parseMarkdown(m_text);
    /*
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    qDebug() << "Parsing took: " << elapsed*1000 << "ms";
    */
    update();
    adjustSize();
}
void LatexLabel::deleteDisplayList(){
    if(m_display_list.empty()) return;
    for (Fragment& f : m_display_list) {
        switch (f.type) {
            case fragment_type::latex:{
                frag_latex_data* data = (frag_latex_data*) f.data;
                delete data->render;
                delete data;
                break;
            }
            case fragment_type::line:{
                frag_line_data* data = (frag_line_data*) f.data;
                delete data;
                break;
            }
            case fragment_type::rounded_rect:{
                frag_rrect_data* data = (frag_rrect_data*) f.data;
                delete data;
                break;
            }
            case fragment_type::text:{
                frag_text_data* data = (frag_text_data*) f.data;
                delete data;
                break;
            }
        }

    }
    m_display_list.clear();
}

void LatexLabel::setText(QString text){
    m_raw_text.clear();
    deleteDisplayList();
    clear_code_block_widgets();
    m_display_list.clear();
    m_text = text;
    clock_t start = clock();
    parseMarkdown(m_text);
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    qDebug() << "Parsing took: " << elapsed*1000 << "ms";



    update();
    adjustSize();
}

void LatexLabel::clear_code_block_widgets(){
    for(auto* widget : m_code_widgets){
        if(widget){ widget->deleteLater(); }
    }
    m_code_widgets.clear();
}

void LatexLabel::create_code_block_widget(const QRect& rect, const QString& text, const QFont& font, const QString& language){
    CodeBlockWidget* widget = new CodeBlockWidget(text,m_textSize, language, this);
    widget->setGeometry(rect.adjusted(8, 8, -8, -8));
    widget->show();

    m_code_widgets.push_back(widget);
}

void LatexLabel::printSegmentsStructure() const {
    qDebug() << "=== m_segments Structure ===";
    for(size_t i = 0; i < m_segments.size(); i++) {
        qDebug() << QString("m_segments[%1]:").arg(i);
        printSegmentRecursive(m_segments[i], 0);
    }
    qDebug() << "=== End Structure ===";
}

void LatexLabel::printSegmentRecursive(const Element* element, int depth) const {
    if(!element) {
        QString indent = QString("  ").repeated(depth);
        qDebug().noquote() << QString("%1null element").arg(indent);
        return;
    }

    QString indent = QString("  ").repeated(depth);
    QString typeStr;

    switch(element->type) {
        case DisplayType::block:
            typeStr = "Block";
            break;
        case DisplayType::span:
            typeStr = "Span";
            break;
    }

    qDebug().noquote() << QString("%1Element {").arg(indent);
    qDebug().noquote() << QString("%1  type: %2").arg(indent, typeStr);

    if(element->type == DisplayType::block) {
        QString blockTypeStr;
        MD_BLOCKTYPE blockType = BLOCKTYPE(element);
        switch(blockType) {
            case MD_BLOCK_DOC: blockTypeStr = "Document"; break;
            case MD_BLOCK_QUOTE: blockTypeStr = "Quote"; break;
            case MD_BLOCK_UL: blockTypeStr = "UnorderedList"; break;
            case MD_BLOCK_OL: blockTypeStr = "OrderedList"; break;
            case MD_BLOCK_LI: blockTypeStr = "ListItem"; break;
            case MD_BLOCK_HR: blockTypeStr = "HorizontalRule"; break;
            case MD_BLOCK_H: blockTypeStr = "Heading"; break;
            case MD_BLOCK_CODE: blockTypeStr = "CodeBlock"; break;
            case MD_BLOCK_HTML: blockTypeStr = "HtmlBlock"; break;
            case MD_BLOCK_P: blockTypeStr = "Paragraph"; break;
            case MD_BLOCK_TABLE: blockTypeStr = "Table"; break;
            case MD_BLOCK_THEAD: blockTypeStr = "TableHead"; break;
            case MD_BLOCK_TBODY: blockTypeStr = "TableBody"; break;
            case MD_BLOCK_TR: blockTypeStr = "TableRow"; break;
            case MD_BLOCK_TH: blockTypeStr = "TableHeader"; break;
            case MD_BLOCK_TD: blockTypeStr = "TableData"; break;
        }
        qDebug().noquote() << QString("%1  blockType: %2").arg(indent, blockTypeStr);

        if(blockType == MD_BLOCK_LI) {
            list_item_data data = std::get<list_item_data>(element->data);
            qDebug().noquote() << QString("%1  listItemData: {").arg(indent);
            qDebug().noquote() << QString("%1    isOrdered: %2").arg(indent).arg(data.is_ordered ? "true" : "false");
            qDebug().noquote() << QString("%1    itemIndex: %2").arg(indent).arg(data.item_index);
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if(blockType == MD_BLOCK_H) {
            heading_data data = std::get<heading_data>(element->data);
            qDebug().noquote() << QString("%1  headingData: {").arg(indent);
            qDebug().noquote() << QString("%1    level: %2").arg(indent).arg(data.level);
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if((blockType == MD_BLOCK_UL || blockType == MD_BLOCK_OL)) {
            list_data data = std::get<list_data>(element->data);
            qDebug().noquote() << QString("%1  listData: {").arg(indent);
            qDebug().noquote() << QString("%1    isOrdered: %2").arg(indent).arg(data.is_ordered ? "true" : "false");
            if(data.is_ordered) {
                qDebug().noquote() << QString("%1    startIndex: %2").arg(indent).arg(data.start_index);
            }
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if(blockType == MD_BLOCK_CODE) {
            code_block_data data = std::get<code_block_data>(element->data);
            qDebug().noquote() << QString("%1  codeBlockData: {").arg(indent);
            qDebug().noquote() << QString("%1    language: \"%2\"").arg(indent, data.language);
            qDebug().noquote() << QString("%1  }").arg(indent);
        }
    }

    if(element->type == DisplayType::span) {
        QString spanTypeStr;
        spantype sType = SPANTYPE(element);
        switch(sType) {
            case spantype::italic: spanTypeStr = "Italic"; break;
            case spantype::bold: spanTypeStr = "Bold"; break;
            case spantype::hyperlink: spanTypeStr = "Link"; break;
            case spantype::image: spanTypeStr = "Image"; break;
            case spantype::code: spanTypeStr = "Code"; break;
            case spantype::strikethrough: spanTypeStr = "Strikethrough"; break;
            case spantype::latex: spanTypeStr = "Latex"; break;
            case spantype::underline: spanTypeStr = "Underline"; break;
            case spantype::normal: spanTypeStr = "Normal"; break;
            case spantype::linebreak: spanTypeStr = "LineBreak"; break;
            case spantype::italic_bold: spanTypeStr = "Italic & Bold"; break;
        }
        qDebug().noquote() << QString("%1  spanType: %2").arg(indent, spanTypeStr);

        if(sType == spantype::hyperlink) {
            link_data data = std::get<link_data>(element->data);
            qDebug().noquote() << QString("%1  linkData: {").arg(indent);
            qDebug().noquote() << QString("%1    url: \"%2\"").arg(indent, data.url);
            qDebug().noquote() << QString("%1    title: \"%2\"").arg(indent, data.title);
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if(sType == spantype::latex) {
            latex_data data = std::get<latex_data>(element->data);
            qDebug().noquote() << QString("%1  latexData: {").arg(indent);
            qDebug().noquote() << QString("%1    isInline: %2").arg(indent).arg(data.isInline ? "true" : "false");
            if(data.render) {
                qDebug().noquote() << QString("%1    render: (width: %2, height: %3)")
                    .arg(indent)
                    .arg(data.render->getWidth())
                    .arg(data.render->getHeight());
            } else {
                qDebug().noquote() << QString("%1    render: null").arg(indent);
            }
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if((sType == spantype::normal || sType == spantype::code)) {
            span_data data = std::get<span_data>(element->data);
            QString contentStr = data.text;
            if(contentStr.length() > 50) {
                contentStr = contentStr.left(47) + "...";
            }
            contentStr = contentStr.replace('\n', "\\n").replace('\t', "\\t");
            qDebug().noquote() << QString("%1  text: \"%2\"").arg(indent, contentStr);
        }
    }

    if(!element->children.empty()) {
        qDebug().noquote() << QString("%1  children: [").arg(indent);
        for(size_t i = 0; i < element->children.size(); i++) {
            if(i > 0) qDebug().noquote() << QString("%1    ,").arg(indent);
            printSegmentRecursive(element->children[i], depth + 2);
        }
        qDebug().noquote() << QString("%1  ]").arg(indent);
    }

    qDebug().noquote() << QString("%1}").arg(indent);
}

void LatexLabel::mousePressEvent(QMouseEvent* event) {
    if(m_selected){
        //remove selection
        m_selected->is_highlighted=false;
        QRect selected_bb=m_selected->bounding_box;
        m_selected=nullptr;
        update(selected_bb);
    }

}
void LatexLabel::mouseMoveEvent(QMouseEvent* event) {

}
void LatexLabel::mouseReleaseEvent(QMouseEvent* event) {
}
void LatexLabel::mouseDoubleClickEvent(QMouseEvent* event){
    for(Fragment& f :m_display_list){
        if(f.type==fragment_type::line||
            f.type==fragment_type::rounded_rect||
            !f.bounding_box.contains(event->pos()))
            continue;
        f.is_highlighted=true;
        m_selected=&f;

        update(f.bounding_box);
        break;
    }

    // Make sure the widget gets focus when clicked
    setFocus();
}
void LatexLabel::keyPressEvent(QKeyEvent* event){
    if(event->matches(QKeySequence::Copy)&&m_selected){
        QString selected_text="error: can't copy this element";
        if(m_selected->type==fragment_type::latex){
            selected_text=((latex_data*)m_selected->data)->text;
        }
        else if(m_selected->type==fragment_type::text){
            selected_text=((frag_text_data*) m_selected->data)->text;
        }
        QGuiApplication::clipboard()->setText(selected_text);
    }

    // Call parent implementation for other keys
    QWidget::keyPressEvent(event);
}

// Fragment creation helper methods for better readability
void LatexLabel::addText(qreal x, qreal y, qreal width, qreal height, const QString& text, const QFont& font) {
    m_display_list.push_back(Fragment(QRect(x, y, width, height), text, font));
}

void LatexLabel::addLatex(qreal x, qreal y, qreal width, qreal height, tex::TeXRender* render, const QString& text, bool isInline) {
    m_display_list.push_back(Fragment(QRect(x, y, width, height), render, text, isInline));
}

void LatexLabel::addLine(qreal x, qreal y, qreal width, qreal height, const QPoint& to, int lineWidth) {
    m_display_list.push_back(Fragment(QRect(x, y, width, height), to, lineWidth));
}

void LatexLabel::addRoundedRect(qreal x, qreal y, qreal width, qreal height, const QRect& rect, qreal radius, QPalette::ColorRole bg) {
    m_display_list.push_back(Fragment(QRect(x, y, width, height), rect, radius, bg));
}

void LatexLabel::addRoundedRect(qreal x, qreal y, qreal width, qreal height, const QRect& rect, qreal tl, qreal tr, qreal bl, qreal br, QPalette::ColorRole bg) {
    m_display_list.push_back(Fragment(QRect(x, y, width, height), rect, tl, tr, bl, br, bg));
}
