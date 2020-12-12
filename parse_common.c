#include "parse_common.h"
#include "lexer.h"

void parse_state_init(parse_state *state, obtaint_fn *obtaint, void *obtaint_arg) {
    state->lookahead_vld = 0;
    state->obtaint = obtaint;
    state->obtaint_arg = obtaint_arg; //Will probably be a lexer_state*
}

//If lookahead is not available, will use the obtain_token
//function to get it
nonterm_t peek_nonterm(parse_state *state) {
    if (!state->lookahead_vld) {
        state->obtaint(&(state->lookahead), state->obtaint_arg);
        state->lookahead_vld = 1;
    }

    switch(state->lookahead.type) {
    //TODO: are labels expressions?
    case TOK_IDENT: case TOK_NUMBER: case TOK_STR: 
        return NT_EXPR;
    case TOK_EOS:
        return NT_EOS;
    case TOK_ERROR:
        return NT_ERROR;
    case TOK_KW: {
        kw_t type = state->lookahead.as_kw;
        if      (IS_STMT_KW(type))               return NT_STMT;
        else if (IS_TYPE_DECL_KW(type))          return NT_DECL;
        else if (IS_UOP_KW(type) || type == '(') return NT_EXPR;
        else if (IS_UOP_KW(type) || type == ';') return NT_STMT;
        else                                     return NT_ERROR;
    }
    default:
        return NT_ERROR;
    }
}