#include "element.h"
#include <cstdlib>
#include <md4c.h>
#include <iostream>
#include <variant>
Element::Element(DisplayType type, ElementData data,spantype span_type, MD_BLOCKTYPE block_type): type(type),data(data){
    if(type==DisplayType::block){
        subtype = malloc(sizeof(MD_BLOCKTYPE));
        *(MD_BLOCKTYPE*)subtype = block_type;
        switch(block_type) {
            case MD_BLOCK_H:
                this->data= heading_data();
                break;
            case MD_BLOCK_CODE:
            this->data= code_block_data();
                break;
            case MD_BLOCK_UL:
            case MD_BLOCK_OL:
                this->data=list_data();
                break;
            case MD_BLOCK_LI:
                this->data=list_item_data();
                break;
            default:
                break;
        }
    }
    else{//span element
        subtype = malloc(sizeof(spantype));
        *(spantype*)subtype = span_type;
        switch(span_type){
            case image://image not supported yet
                break;
            case hyperlink:
                this->data=std::get<link_data>(data);
                break;
            case latex:
                this->data=std::get<latex_data>(data);
                break;
            default:
                if(std::holds_alternative<std::monostate>(data)){
                    this->data = span_data();
                    break;
                }
                this->data= std::get<span_data>(data);
        }
    }
}

Element::~Element(){
    free(subtype);
    for (Element* child : children) {
        delete child;
    }
}

// Helper function to convert MD_BLOCKTYPE to string
const char* blockTypeToString(MD_BLOCKTYPE blockType) {
    switch(blockType) {
        case MD_BLOCK_DOC: return "DOC";
        case MD_BLOCK_QUOTE: return "QUOTE";
        case MD_BLOCK_UL: return "UL";
        case MD_BLOCK_OL: return "OL";
        case MD_BLOCK_LI: return "LI";
        case MD_BLOCK_HR: return "HR";
        case MD_BLOCK_H: return "H";
        case MD_BLOCK_CODE: return "CODE";
        case MD_BLOCK_HTML: return "HTML";
        case MD_BLOCK_P: return "P";
        case MD_BLOCK_TABLE: return "TABLE";
        case MD_BLOCK_THEAD: return "THEAD";
        case MD_BLOCK_TBODY: return "TBODY";
        case MD_BLOCK_TR: return "TR";
        case MD_BLOCK_TH: return "TH";
        case MD_BLOCK_TD: return "TD";
        default: return "UNKNOWN";
    }
}

// Helper function to convert spantype to string
const char* spanTypeToString(spantype spanType) {
    switch(spanType) {
        case normal: return "normal";
        case italic: return "italic";
        case bold: return "bold";
        case italic_bold: return "italic_bold";
        case image: return "image";
        case code: return "code";
        case hyperlink: return "hyperlink";
        case strikethrough: return "strikethrough";
        case underline: return "underline";
        case linebreak: return "linebreak";
        case latex: return "latex";
        default: return "unknown";
    }
}

std::ostream& operator<<(std::ostream& os, const Element& element) {
    if (element.type == DisplayType::block) {
        MD_BLOCKTYPE blockType = *((MD_BLOCKTYPE*)element.subtype);
        os << "Block[" << blockTypeToString(blockType) << "]";

        // Print specific block data
        std::visit([&os](const auto& data) {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                os << " (no data)";
            } else if constexpr (std::is_same_v<T, heading_data>) {
                os << " level=" << data.level;
            } else if constexpr (std::is_same_v<T, code_block_data>) {
                os << " lang=\"" << data.language.toStdString() << "\"";
            } else if constexpr (std::is_same_v<T, list_data>) {
                os << " ordered=" << (data.is_ordered ? "true" : "false");
                if (data.is_ordered) {
                    os << " start=" << data.start_index;
                } else {
                    os << " mark='" << data.mark << "'";
                }
            } else if constexpr (std::is_same_v<T, list_item_data>) {
                os << " index=" << data.item_index << " ordered=" << (data.is_ordered ? "true" : "false");
            }
        }, element.data);
    } else {
        spantype spanType = *((spantype*)element.subtype);
        os << "Span[" << spanTypeToString(spanType) << "]";

        // Print specific span data
        std::visit([&os](const auto& data) {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                os << " (no data)";
            } else if constexpr (std::is_same_v<T, span_data>) {
                os << " text=\"" << data.text.toStdString() << "\"";
            } else if constexpr (std::is_same_v<T, link_data>) {
                os << " url=\"" << data.url.toStdString() << "\" title=\"" << data.title.toStdString() << "\"";
            } else if constexpr (std::is_same_v<T, latex_data>) {
                os << " text=\"" << data.text.toStdString() << "\" inline=" << (data.isInline ? "true" : "false");
            }
        }, element.data);
    }

    // Print children count
    if (!element.children.empty()) {
        os << " children=" << element.children.size();
    }

    return os;
}
