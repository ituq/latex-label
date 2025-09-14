#pragma once

#include <md4c.h>
#include <variant>
#include <vector>
#include <QString>
#include <latex.h>
#include <iostream>

#define BLOCKTYPE(b) *((MD_BLOCKTYPE*)b->subtype)
#define SPANTYPE(b) *((spantype*)b->subtype)

struct list_data{
    char mark; // mark delimiter if ordered
    bool is_ordered = false;
    uint16_t start_index =1; // 16 bits for aligment, default is 1
};
struct list_item_data{
    int item_index;
    bool is_ordered;
};

struct heading_data{
    int level;
};
struct code_block_data{
    QString language;
};

struct span_data{
public:
    QString text;
};
struct link_data{
    QString title;
    QString url;
};
struct latex_data{
    tex::TeXRender* render;
    QString text;
    bool isInline;
};

typedef enum DisplayType{
    block,
    span
} DisplayTypeType;
typedef enum spantype{
    normal,
    italic,
    bold,
    italic_bold,
    image,
    code,
    hyperlink,
    strikethrough,
    underline,
    linebreak,
    latex
} SpanType;
using ElementData = std::variant<
    std::monostate,
    list_data,
    list_item_data,
    heading_data,
    code_block_data,
    span_data,
    link_data,
    latex_data
>;

class Element{

public:
Element(DisplayType type, ElementData data = {}, spantype span_type =spantype::normal, MD_BLOCKTYPE block_type = MD_BLOCKTYPE::MD_BLOCK_P);
~Element();
DisplayType type;
ElementData data;
void* subtype;
std::vector<Element*> children;

friend std::ostream& operator<<(std::ostream& os, const Element& element);
};

std::ostream& operator<<(std::ostream& os, const Element& element);
