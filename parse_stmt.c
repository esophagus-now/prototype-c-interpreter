#include "parse_common.h"
#include "parse_stmt.h"

#include <stdio.h>


//Dummy statement parser: just skips over semicolons
//Returns 0 on success, negative otehrwise
int parse_stmt(parse_state *state) {
    if (state->lookahead.type == TOK_KW && state->lookahead.as_kw == ';') {
        //state->obtaint(&state->lookahead, state->obtaint_arg);
        state->lookahead_vld = 0;
        return 0;
    } else return -1;
}