#ifndef PARSE_EXPR_H
#define PARSE_EXPR_H 1

#include "parse_common.h"
#include "ast.h"

int parse_expr(parse_state *state, kw_t last_bop, ast *a, ast_node **n_out);

#endif