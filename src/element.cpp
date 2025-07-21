#include "element.h"
#include <cstdlib>
#include <md4c.h>
Element::Element(DisplayType type, void* data,spantype span_type, MD_BLOCKTYPE block_type): type(type),data(data){
    if(type==DisplayType::block){
        subtype = malloc(sizeof(MD_BLOCKTYPE));
        *(MD_BLOCKTYPE*)subtype = block_type;
    }
    else{
        subtype = malloc(sizeof(spantype));
        *(spantype*)subtype = span_type;
    }
}

Element::~Element(){
    //Note: data is freed by cleanup_segments in LatexLabel, not here
    //Note: children are managed as pointers in cleanup_segments
}
