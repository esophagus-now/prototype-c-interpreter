#ifndef PARSE_COMMON_H
#define PARSE_COMMON_H  1

#include "lexer.h"

typedef int obtaint_fn(token *, void *);

typedef struct {
    token lookahead;
    int lookahead_vld;

    obtaint_fn *obtaint;
    void *obtaint_arg;
} parse_state;

typedef enum {
    NT_EOS,
    NT_ERROR,
    NT_STMT,
    NT_DECL,
    NT_EXPR,
} nonterm_t;

void parse_state_init(parse_state *state, obtaint_fn *obtaint, void *obtaint_arg);

//If lookahead is not available, will use the obtain_token
//function to get it
nonterm_t peek_nonterm(parse_state *state);

#endif