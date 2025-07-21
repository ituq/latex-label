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
#include <md4c.h>

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
    //Clean up all segments and their children
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

tex::TeXRender* getLatexRenderer(const QString& latex, bool isInline, int text_size) {
    try {
        tex::Formula formula;
        formula.setLaTeX(latex.toStdWString());
        float width = isInline ? 280 : 600;
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
    Element* block = new Element(DisplayType::block);
    block->subtype=malloc(sizeof(MD_BLOCKTYPE));
    *((MD_BLOCKTYPE*)block->subtype) = type;
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
            list_data* data = (list_data*) malloc(sizeof(list_data));
            data->is_ordered = false;
            if(detail){
                MD_BLOCK_UL_DETAIL* ul_detail = (MD_BLOCK_UL_DETAIL*) detail;

                data->mark=ul_detail->mark;
            }
            block->data=(void*) data;
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_OL:{
            list_data* data = (list_data*) malloc(sizeof(list_data));
            data->is_ordered = true;
            if(detail) {
                MD_BLOCK_OL_DETAIL* ol_detail = (MD_BLOCK_OL_DETAIL*) detail;
                data->start_index = (uint16_t) ol_detail->start;
                data->mark= ol_detail->mark_delimiter;
            }
            block->data=(void*) data;
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_LI:{
            list_item_data* data = (list_item_data*) malloc(sizeof(list_item_data));
            Element* parent = state->blockStack.back();
            data->is_ordered = ((list_data*)parent->data)->is_ordered;
            data->item_index = parent->children.size()+1;
            block->data=(void*) data;
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
            heading_data* data = new heading_data();
            if(detail) {
                MD_BLOCK_H_DETAIL* h_detail = (MD_BLOCK_H_DETAIL*) detail;
                data->level = h_detail->level;
            }
            else{
                data->level = 1;
            }
            block->data=(void*) data;
            state->blockStack.back()->children.push_back(block);
            state->blockStack.push_back(block);
            break;
        }
        case MD_BLOCK_CODE:{
            code_block_data* data = new code_block_data();
            if(detail) {
                MD_BLOCK_CODE_DETAIL* code_detail = (MD_BLOCK_CODE_DETAIL*)(detail);
                if(code_detail->lang.text) {
                    data->language = QString::fromUtf8(code_detail->lang.text, code_detail->lang.size);
                }
            }
            block->data=(void*) data;
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
    Element* span = new Element(DisplayType::span);
    span->subtype=malloc(sizeof(spantype));
    spantype* subtype = (spantype*)span->subtype;

    switch(type) {
        case MD_SPAN_EM:
            *subtype=spantype::italic;
            span->data= malloc(sizeof(span_data));
            break;
        case MD_SPAN_STRONG:
            *subtype=spantype::bold;
            span->data= malloc(sizeof(span_data));
            break;
        case MD_SPAN_A:{

            *subtype=spantype::link;
            link_data* data = (link_data*)malloc(sizeof(link_data));

            if(detail) {
                MD_SPAN_A_DETAIL* a_detail = static_cast<MD_SPAN_A_DETAIL*>(detail);
                if(a_detail->href.text) {
                    data->url = QString::fromUtf8(a_detail->href.text, a_detail->href.size);
                }
                if(a_detail->title.text) {
                    data->title = QString::fromUtf8(a_detail->title.text, a_detail->title.size);
                }
            }
            else{
                data->url = "Error parsing link";
                data->title = "Error parsing link";
            }
            span->data = data;
            state->blockStack.back()->children.push_back(span);
            return 0;
        }

            break;
        case MD_SPAN_IMG:
            *subtype=spantype::image;
            qDebug()<<"images are not supported yet";
            span->data=nullptr;
            break;
        case MD_SPAN_CODE:
            *subtype=spantype::code;
            span->data= malloc(sizeof(span_data));
            break;
        case MD_SPAN_DEL:
            *subtype=spantype::strikethrough;
            span->data= malloc(sizeof(span_data));
            break;
        case MD_SPAN_LATEXMATH:{
            *subtype=spantype::latex;
            latex_data* data = (latex_data*) malloc(sizeof(latex_data));
            data->isInline=true;
            span->data=data;
        }
            break;

        case MD_SPAN_LATEXMATH_DISPLAY:{
            *subtype=spantype::latex;
            latex_data* data = (latex_data*) malloc(sizeof(latex_data));
            data->isInline=false;
            span->data=data;
        }
            break;
        case MD_SPAN_WIKILINK:
            qDebug()<<"wiki links are not supported yet";
            break;
        case MD_SPAN_U:
            *subtype=spantype::underline;
            span->data= malloc(sizeof(span_data));
            break;
    }


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
        case MD_SPAN_A:
            return 0;
        case MD_SPAN_IMG:
            return 0;
        case MD_SPAN_WIKILINK:
            return 0;
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
    if( state->spanStack.empty() ){ //no open span, add to recent block element
        Element* block= state->blockStack.back();

        span_data* data = (span_data*) malloc(sizeof(span_data));
        switch(type) {
            case MD_TEXT_NORMAL:{

                Element* t = new Element(DisplayType::span,data,spantype::normal);
                data->text = textStr;
                block->children.push_back(t);

            }
                break;
            case MD_TEXT_NULLCHAR:{


                Element* t = new Element(DisplayType::span,data,spantype::normal);
                data->text = QChar(0xFFFD); // Unicode replacement character
                block->children.push_back(t);
            }
                break;
            case MD_TEXT_BR:{

                Element* t = new Element(DisplayType::span,data,spantype::linebreak);
                block->children.push_back(t);
            }
                break;
            case MD_TEXT_SOFTBR:
                qDebug()<<"ignoring soft break";
                return 0; // ignore soft break
                break;
            case MD_TEXT_ENTITY:
                qDebug()<<"entity is not supported yet";
                return 0; // ignore entity
                break;
            case MD_TEXT_CODE:{
                data->text = textStr;
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
    span_data* data = (span_data*) parent_span->data;

    switch(type) {
        case MD_TEXT_NORMAL:
            data->text=textStr;
            break;
        case MD_TEXT_NULLCHAR:
            data->text = QChar(0xFFFD); // Unicode replacement character
            break;
        case MD_TEXT_BR: //we change the span type to a linebreak
            *((spantype*)parent_span->subtype)=spantype::linebreak;
            break;
        case MD_TEXT_SOFTBR:// do nothing

            break;
        case MD_TEXT_ENTITY: //do nothing
            break;
        case MD_TEXT_CODE:
            data->text=textStr;
            break;
        case MD_TEXT_HTML://do nothing
            break;
        case MD_TEXT_LATEXMATH:
            {
            latex_data* data_latex = (latex_data*)parent_span->data;
            data_latex->render=getLatexRenderer(textStr, data_latex->isInline,state->textSize);
            data_latex->text=textStr;
            }
            break;
    }


    return 0;
}

void LatexLabel::cleanup_segments(std::vector<Element*>& segments) {
    for(Element* elem : segments) {
        if(elem == nullptr) continue;

        //Clean up LaTeX render objects (these are created with getLatexRenderer)
        if(elem->type==DisplayType::span&& *((spantype*)elem->subtype) ==spantype::latex) {
            latex_data* data = (latex_data*) elem->data;
            if(data->render != nullptr) {
                delete data->render;
                data->render = nullptr;
            }
        }

        //Recursively clean up children for block elements
        if(elem->type==DisplayType::block) {
            if(!elem->children.empty()) {
                cleanup_segments(elem->children);
                elem->children.clear();
            }
        }

        //Delete the element itself
        delete elem;
    }
    m_segments.clear();
}

void LatexLabel::parseMarkdown(const QString& text) {
    //Clean up previous segments
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
        //Move the parsed AST to m_segments
        m_segments = std::move(state.segments);
    } else {
        //Clean up any partial parsing results
        cleanup_segments(state.segments);
        qDebug() << "Markdown parsing failed, result code:" << result;
    }
}


QFont LatexLabel::getFont(const Element* segment) const {
    QFont font("Arial", m_textSize);

    if(segment->type==DisplayType::block&&*((MD_BLOCKTYPE*)segment->subtype)==MD_BLOCK_H){
        heading_data* data = (heading_data*) segment->data;
        int headingSize = std::max(8, m_textSize + (6 - data->level) * 4);
        font.setPointSize(headingSize);
        font.setBold(true);
    }
    else if (segment->type==DisplayType::block&&*((MD_BLOCKTYPE*)segment->subtype)==MD_BLOCK_CODE){
        font.setFamily("Monaco");
    }
    else if(segment->type==DisplayType::span){

        switch (*(spantype*)segment->subtype) {
            case spantype::bold:
                font.setBold(true);
                break;
            case spantype::italic:
                font.setItalic(true);
                break;
            case spantype::underline:
                font.setUnderline(true);
                break;
            case spantype::strikethrough:
                font.setStrikeOut(true);
                break;
            case spantype::code:
                font.setFamily("Monaco");
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

void LatexLabel::renderSpan(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal lineHeight,QFont* font_passed) {
    QFont font;
    if(font_passed){
        font=*font_passed;
    }
    else{
        font = getFont(&segment);
    }
    spantype type = *(spantype*)segment.subtype;
    QColor color = type==spantype::link ? Qt::blue : Qt::black;

    painter.setFont(font);
    painter.setPen(color);

    QFontMetricsF metrics(font);
    //Remember the left margin for line wrapping
    //m_leftMargin = x;


    // Handle different text types

    if(type == spantype::linebreak) {
        y += lineHeight+m_leading;
        return;
    }


    // Handle LaTeX math
    if(type == spantype::latex) {
        latex_data* data = (latex_data*) segment.data;
        qreal renderWidth = data->render->getWidth();
        qreal renderHeight = data->render->getHeight();

        if(data->isInline){
            //inline latex, check if it fits on line
            x = m_leftMargin;
            y += lineHeight+m_leading;

        }
        else{
            y += renderHeight;
            x=min_x+(max_x-min_x)/2-renderWidth/2;
        }

        // Draw LaTeX expression
        painter.save();
        qreal latexY = y - (renderHeight - data->render->getDepth());
        tex::Graphics2D_qt g2(&painter);
        data->render->draw(g2, static_cast<int>(x), static_cast<int>(latexY));
        painter.restore();

        if(data->isInline){
            x += renderWidth + 3.0;
        }
        else{
            x = m_leftMargin;
            y += renderHeight;
        }

        return;
    }

    //regular text
    span_data* data = (span_data*) segment.data;
    QString text = data->text;



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
        if(x + wordWidth > max_x && x > m_leftMargin) {
            x = m_leftMargin;
            y += lineHeight+m_leading;
        }

        painter.drawText(QPointF(x, y), wordWithSpace);
        x += wordWidth;
    }
}


void LatexLabel::renderBlock(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight) {
    QFont font = getFont(&segment);
    QFontMetricsF metrics(font);
    qreal currentLineHeight = getLineHeight(segment, metrics);

    switch(*(MD_BLOCKTYPE*)segment.subtype) {
        case MD_BLOCK_DOC:
            // Document block - render all children
            for(const Element* child : segment.children) {
                if(child->type==DisplayType::block) {
                    renderBlock(painter, *child, x, y, min_x,max_x, lineHeight);
                } else {
                    renderSpan(painter, *child, x, y, min_x,max_x, lineHeight);
                }
            }
            break;
        case MD_BLOCK_H:
            renderHeading(painter, segment, x, y, min_x,max_x, lineHeight);
            break;
        case MD_BLOCK_CODE:
            renderCodeBlock(painter, segment, x, y, min_x,max_x, lineHeight);
            break;
        case MD_BLOCK_QUOTE:
            renderBlockquote(painter, segment, x, y, min_x,max_x, lineHeight);
            break;
        case MD_BLOCK_UL:
        case MD_BLOCK_OL:
        case MD_BLOCK_LI:
            //renderListElement(painter, segment, x, y, min_x,max_x, maxWidth, lineHeight);
            break;
        case MD_BLOCK_TABLE:
            //renderTable(painter, segment, x, y, min_x,max_x, maxWidth, lineHeight);
            break;
        case MD_BLOCK_HR:
            // Draw horizontal line
            y += 2*lineHeight;
            painter.save();
            painter.setPen(QPen(Qt::gray, 2));
            painter.drawLine(min_x+5, y, max_x, y);
            painter.restore();
            y += 2*lineHeight;
            break;
        case MD_BLOCK_P:
            {
                QFont baseFont("Arial", m_textSize);
                for(const Element* child : segment.children) {
                        renderSpan(painter, *child, x, y,min_x,max_x, lineHeight);
                }
                x = min_x;
                y += currentLineHeight;
            }
            break;
        default:
            // Default block rendering
            {
                for(const auto& child : segment.children) {
                    renderSpan(painter, *child, x, y,min_x,max_x, lineHeight);
                }
            }
            break;
    }
}

void LatexLabel::renderListElement(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight) {

    MD_BLOCKTYPE type =*(MD_BLOCKTYPE*)segment.subtype;


    if(type==MD_BLOCK_LI) {
        list_item_data* data =(list_item_data*) segment.data;
        //draw list marker
        painter.save();
        QFont baseFont("Arial", m_textSize);
        painter.setFont(baseFont);
        painter.setPen(Qt::black);


        QString marker;
        if(data->is_ordered) {
            //ordered list - use item number
            marker = QString::number(data->item_index) + ". ";
        } else {
            //unordered list - use bullet
            marker = "• "; //TODO: replace with actual markers
        }


        painter.drawText(QPointF(x, y), marker);

        //adjust x position based on marker width
        QFontMetricsF metrics(baseFont);
        qreal markerWidth = metrics.horizontalAdvance(marker);
        x += markerWidth + 2.0;

        painter.restore();
        qreal left_border = x;



        // Render list item content
        QFont listFont("Arial", m_textSize);
        for(const Element* child : segment.children) {
            if(child->type==DisplayType::span) {
                renderSpan(painter, *child, x, y, left_border,max_x, lineHeight);
            }
            else{
                renderBlock(painter, *child, x, y, left_border,max_x, lineHeight);
            }
        }
        y += lineHeight * 1.3;
        x = min_x;
    } else {
        // List container - render children
        for(const Element* child : segment.children) {
            if(child->type==DisplayType::block) {
                renderBlock(painter, *child, x, y, min_x,max_x, lineHeight);
            }
        }
        // Add spacing after the entire list
        x=min_x;
    }
}

void LatexLabel::renderHeading(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight) {
    painter.save();

    QFont font = getFont(&segment);
    QFontMetricsF metrics(font);
    painter.setFont(font);
    painter.setPen(Qt::black);

    qreal headingLineHeight = getLineHeight(segment, metrics);

    // Add more spacing before heading (move to next line if not at start)
    if(x > min_x) {
        x = min_x;
        y += lineHeight;
    }
    y += headingLineHeight * 0.8; // Increased spacing before heading


    // Render heading content with heading font inherited
    for(const Element* child : segment.children) {
        renderSpan(painter, *child, x, y, min_x,max_x, headingLineHeight,&font);
    }

    // Add spacing after heading and move to next line
    x = min_x;
    y += headingLineHeight * 0.8; // Increased spacing after heading

    painter.restore();
}

void LatexLabel::renderCodeBlock(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x,qreal max_x, qreal& lineHeight) {
    painter.save();

    // Set code font
    QFont font = getFont(&segment);
    QFontMetricsF fm(font);
    painter.setFont(font);
    painter.setPen(Qt::white);

    y += 10.0;
    double startY = y;

    // Measure total height first
    qreal tempY = y;
    for(const Element* child : segment.children) {

        if(child->type==DisplayType::span && *(spantype*)child->subtype== spantype::code) {
            QString text = ((span_data*)child->data)->text;
            QStringList lines = text.split('\n');
            tempY += lines.size() * (lineHeight+m_leading);
        }
    }

    // Draw background first
    painter.setPen(Qt::gray);
    painter.setBrush(Qt::white);

    painter.drawRoundedRect(QRectF(min_x, y-fm.ascent(), max_x - 10, tempY - startY),10,10);
    painter.setPen(Qt::black);


    x+=10;
    y+=10;
    // Draw text
    for(const Element* child : segment.children) {
        if(child->type == DisplayType::span) {
            QString text = ((span_data*)child->data)->text;
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

void LatexLabel::renderBlockquote(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal min_x, qreal max_x, qreal& lineHeight) {
    painter.save();
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
            renderBlock(painter, *child, x, y, min_x+50,max_x, lineHeight);
        } else {
            renderSpan(painter, *child, x, y, min_x+50, max_x, lineHeight, &blockquoteFont);
        }
    }

    //draw line left of quote
    painter.setPen(QPen(Qt::gray, 3));
    painter.drawLine(startX-5, startY - metrics.ascent(), startX-5, y-metrics.height() + metrics.descent()); // Draw from start to end of content

    // Reset position and add spacing after blockquote
    x = min_x;
    y += lineHeight * 1.2; // Add more spacing after blockquote

    painter.restore();
}

void LatexLabel::renderTable(QPainter& painter, const Element& segment, qreal& x, qreal& y, qreal maxWidth, qreal& lineHeight) {
    //Convert markdown table to LaTeX table syntax
    /*
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
    */
}

void LatexLabel::paintEvent(QPaintEvent* event){
    //QRectF area_to_redraw = event->rect();
    //clock_t start = clock();

    QPainter painter(this);
    painter.fillRect(rect(), QColor(255, 255, 255)); // White background
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFontMetricsF fontMetrics = painter.fontMetrics();
    qreal lineHeight = fontMetrics.height() * 1.2;
    qreal x = 5.0;
    qreal y = 5.0 + fontMetrics.ascent();  // y represents the text baseline
    qreal maxWidth = width() - 10.0;

    for(const Element* segment : m_segments) {
        // Check if we're still within the widget bounds
        if(y > height() - 5.0) break;
        if(segment->type==DisplayType::block){
            renderBlock(painter, *segment, x, y,5.0,width(), lineHeight);
        }
        else{
            renderSpan(painter, *segment, x, y,5.0,width(), lineHeight);
        }

    }
    widget_height=y+50.0; // Add some padding at the bottom
    setMinimumHeight(widget_height);
    updateGeometry();
    /*
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    qDebug() << "Rendering took: " << elapsed*1000 << "ms";
    */
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
    clock_t start = clock();
    parseMarkdown(m_text);
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    qDebug() << "Parsing took: " << elapsed*1000 << "ms";
    update();
    adjustSize();
}

void LatexLabel::setText(QString text){
    m_text = text;
    clock_t start = clock();
    parseMarkdown(m_text);
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    qDebug() << "Parsing took: " << elapsed << " s";
    //qDebug() << "=== LIST NESTING DEBUG: After parsing ===";
    //printSegmentsStructure();
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
        MD_BLOCKTYPE blockType = *(MD_BLOCKTYPE*)element->subtype;
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

        if(blockType == MD_BLOCK_LI && element->data) {
            list_item_data* data = (list_item_data*)element->data;
            qDebug().noquote() << QString("%1  listItemData: {").arg(indent);
            qDebug().noquote() << QString("%1    isOrdered: %2").arg(indent).arg(data->is_ordered ? "true" : "false");
            qDebug().noquote() << QString("%1    itemIndex: %2").arg(indent).arg(data->item_index);
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if(blockType == MD_BLOCK_H && element->data) {
            heading_data* data = (heading_data*)element->data;
            qDebug().noquote() << QString("%1  headingData: {").arg(indent);
            qDebug().noquote() << QString("%1    level: %2").arg(indent).arg(data->level);
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if((blockType == MD_BLOCK_UL || blockType == MD_BLOCK_OL) && element->data) {
            list_data* data = (list_data*)element->data;
            qDebug().noquote() << QString("%1  listData: {").arg(indent);
            qDebug().noquote() << QString("%1    isOrdered: %2").arg(indent).arg(data->is_ordered ? "true" : "false");
            if(data->is_ordered) {
                qDebug().noquote() << QString("%1    startIndex: %2").arg(indent).arg(data->start_index);
            }
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if(blockType == MD_BLOCK_CODE && element->data) {
            code_block_data* data = (code_block_data*)element->data;
            qDebug().noquote() << QString("%1  codeBlockData: {").arg(indent);
            qDebug().noquote() << QString("%1    language: \"%2\"").arg(indent, data->language);
            qDebug().noquote() << QString("%1  }").arg(indent);
        }
    }

    if(element->type == DisplayType::span) {
        QString spanTypeStr;
        spantype sType = *(spantype*)element->subtype;
        switch(sType) {
            case spantype::italic: spanTypeStr = "Italic"; break;
            case spantype::bold: spanTypeStr = "Bold"; break;
            case spantype::link: spanTypeStr = "Link"; break;
            case spantype::image: spanTypeStr = "Image"; break;
            case spantype::code: spanTypeStr = "Code"; break;
            case spantype::strikethrough: spanTypeStr = "Strikethrough"; break;
            case spantype::latex: spanTypeStr = "Latex"; break;
            case spantype::underline: spanTypeStr = "Underline"; break;
            case spantype::normal: spanTypeStr = "Normal"; break;
            case spantype::linebreak: spanTypeStr = "LineBreak"; break;
        }
        qDebug().noquote() << QString("%1  spanType: %2").arg(indent, spanTypeStr);

        if(sType == spantype::link && element->data) {
            link_data* data = (link_data*)element->data;
            qDebug().noquote() << QString("%1  linkData: {").arg(indent);
            qDebug().noquote() << QString("%1    url: \"%2\"").arg(indent, data->url);
            qDebug().noquote() << QString("%1    title: \"%2\"").arg(indent, data->title);
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if(sType == spantype::latex && element->data) {
            latex_data* data = (latex_data*)element->data;
            qDebug().noquote() << QString("%1  latexData: {").arg(indent);
            qDebug().noquote() << QString("%1    isInline: %2").arg(indent).arg(data->isInline ? "true" : "false");
            if(data->render) {
                qDebug().noquote() << QString("%1    render: (width: %2, height: %3)")
                    .arg(indent)
                    .arg(data->render->getWidth())
                    .arg(data->render->getHeight());
            } else {
                qDebug().noquote() << QString("%1    render: null").arg(indent);
            }
            qDebug().noquote() << QString("%1  }").arg(indent);
        }

        if((sType == spantype::normal || sType == spantype::code) && element->data) {
            span_data* data = (span_data*)element->data;
            QString contentStr = data->text;
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
