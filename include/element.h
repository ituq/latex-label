#include <md4c.h>
#include <vector>
#include <QString>
#include <latex.h>

#define BLOCKTYPE(b) *((MD_BLOCKTYPE*)b->subtype)
#define SPANTYPE(b) *((spantype*)b->subtype)

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

class Element{

public:
Element(DisplayType type, void* data = nullptr, spantype span_type =spantype::normal, MD_BLOCKTYPE block_type = MD_BLOCKTYPE::MD_BLOCK_P);
~Element();
DisplayType type;
void* data;
void* subtype;
std::vector<Element*> children;
};

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
