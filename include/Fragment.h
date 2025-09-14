#pragma once

#include <QWidget>
#include <QString>
#include <QRect>
#include <QPoint>
#include <QFont>
#include <QPalette>
#include <QDebug>
#include <cstdint>
#include <iostream>
#include "latex.h"

enum class fragment_type{
    latex,
    text,
    line,
    rounded_rect,
    clipped_text
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
    QPalette::ColorRole color;

    // Constructor with color
    frag_text_data(QString t, QFont f, QPalette::ColorRole c = QPalette::Text)
        : text(t), font(f), color(c) {}
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
    QPalette::ColorRole stroke;

    // Constructor with individual corner radii
    frag_rrect_data(QRect r, qreal tl, qreal tr, qreal bl, qreal br, QPalette::ColorRole bg, QPalette::ColorRole st = QPalette::WindowText)
        : rect(r), topLeftRadius(tl), topRightRadius(tr), bottomLeftRadius(bl), bottomRightRadius(br), background(bg), stroke(st) {}

    // Constructor with uniform radius for all corners
    frag_rrect_data(QRect r, qreal radius, QPalette::ColorRole bg, QPalette::ColorRole st = QPalette::WindowText)
        : rect(r), topLeftRadius(radius), topRightRadius(radius), bottomLeftRadius(radius), bottomRightRadius(radius), background(bg), stroke(st) {}
};

struct frag_latex_data{
    tex::TeXRender* render;
    QString text;
    bool isInline;
};
struct clipped_text_data{
    QRect clipArea;
    QString text;
    uint8_t codeBlock_id;
};

typedef struct Fragment{
    QRect bounding_box;
    fragment_type type;
    bool is_highlighted;
    void* data;

    Fragment(QRect bounding_box,QString text, QFont font, QPalette::ColorRole color = QPalette::Text):bounding_box(bounding_box),is_highlighted(false),type(fragment_type::text){
        //text constructor
        data = new frag_text_data(text,font,color);
    }
    Fragment(QRect bounding_box, QRect rect,int radius,QPalette::ColorRole bg, QPalette::ColorRole stroke = QPalette::WindowText):bounding_box(bounding_box),is_highlighted(false),type(fragment_type::rounded_rect){
        //rounded rect constructor
        data = new frag_rrect_data(rect,radius,bg,stroke);
    }
    Fragment(QRect bounding_box, QRect rect, qreal tl, qreal tr, qreal bl, qreal br, QPalette::ColorRole bg, QPalette::ColorRole stroke = QPalette::WindowText):bounding_box(bounding_box),is_highlighted(false),type(fragment_type::rounded_rect){
        //rounded rect constructor with individual corner radii
        data = new frag_rrect_data(rect,tl,tr,bl,br,bg,stroke);
    }
    Fragment(QRect bounding_box, QPoint to, int width = 1): bounding_box(bounding_box),is_highlighted(false), type(fragment_type::line){
        //line constructor
        data = new frag_line_data(to,width);
    }
    Fragment(QRect bounding_box, tex::TeXRender* render, QString text, bool isInline): bounding_box(bounding_box),is_highlighted(false), type(fragment_type::latex){
        data = new frag_latex_data(render,text,isInline);
    }
    Fragment(QRect clip_area,QRect bounding, QString& text,int id) : bounding_box(bounding),is_highlighted(false),type(fragment_type::clipped_text){
        //clipped text constructor

        data = new clipped_text_data(clip_area, text,id);
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
            case fragment_type::clipped_text:
                result += "clipped_text";
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
                result += QString(", color: %1").arg(text_data->color);
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
                result += QString(", bg: %1, stroke: %2").arg(rect_data->background).arg(rect_data->stroke);
                break;
            }
            case fragment_type::clipped_text: {
                clipped_text_data* clip_data = (clipped_text_data*)fragment.data;
                QString clip_text = clip_data->text;
                if(clip_text.length() > 50) {
                    clip_text = clip_text.left(47) + "...";
                }
                clip_text = clip_text.replace('\n', "\\n").replace('\t', "\\t");
                result += QString(", text: \"%1\"").arg(clip_text);

                result += QString(", clip_area: (%1,%2,%3,%4)")
                            .arg(clip_data->clipArea.x())
                            .arg(clip_data->clipArea.y())
                            .arg(clip_data->clipArea.width())
                            .arg(clip_data->clipArea.height());
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
        case fragment_type::clipped_text:
            result += "clipped_text";
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
            result += QString(", color: %1").arg(text_data->color);
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
            result += QString(", bg: %1, stroke: %2").arg(rect_data->background).arg(rect_data->stroke);
            break;
        }
        case fragment_type::clipped_text: {
            clipped_text_data* clip_data = (clipped_text_data*)fragment.data;
            QString clip_text = clip_data->text;
            if(clip_text.length() > 50) {
                clip_text = clip_text.left(47) + "...";
            }
            clip_text = clip_text.replace('\n', "\\n").replace('\t', "\\t");
            result += QString(", text: \"%1\"").arg(clip_text);
            result += QString(", clip_area: (%1,%2,%3,%4)")
                        .arg(clip_data->clipArea.x())
                        .arg(clip_data->clipArea.y())
                        .arg(clip_data->clipArea.width())
                        .arg(clip_data->clipArea.height());
            break;
        }
    }

    result += "}";
    os << result.toStdString();
    return os;
}
