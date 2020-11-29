#ifndef PARSE_TYPE_H 
#define PARSE_TYPE_H 1

#include "parse_common.h"
#include "type.h"
#include "util/vector.h"

//TODO: should this handle typedefs, or should I have a separate
//function for that?
//TODO: structs, unions, enums
//TODO: write another function to parse initializers and compound
//literals
//TODO: qualifiers
//(this is unending...)
//Returs 0 on success, negative otherwise
int parse_type(parse_state *state, VECTOR_PTR_PARAM(tq, tstr));

#endif