#ifndef PARSE_STMT_H
#define PARSE_STMT_H 1

#include "parse_common.h"

//Dummy statement parser: just skips over semicolons
//Returns 0 on success, negative otehrwise
int parse_stmt(parse_state *state);


#endif