#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "parse_common.h"
#include "parse_expr.h"
#include "ast.h"
#include "lexer.h"

//TODO: sizeof operator

//For everything else in this frontend I did it myself
//as a challenge to see how well I knew C. But the operator
//precedence is so complicated I just looked it up:
//https://en.cppreference.com/w/c/language/operator_precedence

//In that chart, I get precedence level 1 for free just because 
//of the way I coded my parser (TODO: compound literals and struct
//member access).
//I also get precedence level 13 (for the ternary op) for free 
//because of how I wrote the code.
//Finally, I get all the unary operators for free, again because
//of the way I wrote the parser

static int const bop_prec[] = {
    ['\0'] = 0, //Sentinel
    [','] = 1,
    ['='] = 2,
    [KW_plus_eq] = 2,
    [KW_minus_eq] = 2,
    [KW_times_eq] = 2,
    [KW_div_eq] = 2,
    [KW_mod_eq] = 2,
    [KW_and_eq] = 2,
    [KW_or_eq] = 2,
    [KW_xor_eq] = 2,
    [KW_lshift_eq] = 2,
    [KW_rshift_eq] = 2,
    [KW_or_or] = 3,
    [KW_and_and] = 4,
    ['|'] = 5,
    ['^'] = 6,
    ['&'] = 7,
    [KW_eq_eq] = 8,
    [KW_neq] = 8,
    ['<'] = 9,
    [KW_leq] = 9,
    ['>'] = 9,
    [KW_geq] = 9,
    [KW_lshift] = 10,
    [KW_rshift] = 10,
    ['+'] = 11,
    ['-'] = 11,
    ['*'] = 12,
    ['/'] = 12,
    ['%'] = 12

};

static int const bop_is_rassoc[] = {
    ['\0'] = 1, //This should never be used
    [','] = 0,
    ['='] = 1,
    [KW_plus_eq] = 1,
    [KW_minus_eq] = 1,
    [KW_times_eq] = 1,
    [KW_div_eq] = 1,
    [KW_mod_eq] = 1,
    [KW_and_eq] = 1,
    [KW_or_eq] = 1,
    [KW_xor_eq] = 1,
    [KW_lshift_eq] = 1,
    [KW_rshift_eq] = 1,
    [KW_or_or] = 0,
    [KW_and_and] = 0,
    ['|'] = 0,
    ['^'] = 0,
    ['&'] = 0,
    [KW_eq_eq] = 0,
    [KW_neq] = 0,
    ['<'] = 0,
    [KW_leq] = 0,
    ['>'] = 0,
    [KW_geq] = 0,
    [KW_lshift] = 0,
    [KW_rshift] = 0,
    ['+'] = 0,
    ['-'] = 0,
    ['*'] = 0,
    ['/'] = 0,
    ['%'] = 0
};

#define OBTAINT(state, tok) (state->obtaint(tok, state->obtaint_arg))

//Returns 0 on success, or negative on error
int parse_expr(parse_state *state, kw_t last_bop, ast *a, ast_node **n_out) {
    assert(state->lookahead_vld);

    ast_node *n = NULL;

    //Check FIRST set
    if (state->lookahead.type == TOK_KW && state->lookahead.as_kw == '(') { 
        //E <- (E)
        OBTAINT(state, &(state->lookahead));
        int rc = parse_expr(state, 0, a, &n);
        if (rc < 0) return rc; //Propagate error code
        if (state->lookahead.type != TOK_KW || state->lookahead.as_kw != ')') {
            puts("\nError, unmatched '('");
            return -1;
        }
    } else if (state->lookahead.type == TOK_KW && IS_UOP_KW(state->lookahead.as_kw)) {
        //E <- unary_op E
        kw_t op = state->lookahead.as_kw;
        ast_node *operand_node;
        OBTAINT(state, &(state->lookahead));
        int rc = parse_expr(state, 0, a, &operand_node);
        if (rc < 0) return rc; //Propagate error code

        n = ast_node_alloc(a);
        n->node_type = UNARY_OP;
        n->op = op;
        ast_node_set_child(a, n, 0, operand_node);
    } else if (state->lookahead.type == TOK_IDENT) {
        // E <- ident 
        //TODO: struct member access
        n = ast_node_alloc(a);
        n->node_type = IDENT;
        //FIXME: look up the symbol and put it in the node
        //For the sake of testing,
        strcpy(n->ident_str, state->lookahead.as_ident.name);

        OBTAINT(state, &(state->lookahead));
    } else if (state->lookahead.type == TOK_NUMBER) {
        // E <- literal
        n = ast_node_alloc(a);
        n->node_type = NUMBER;
        //FIXME: stick the value in the ast_node somewhere
        
        OBTAINT(state, &(state->lookahead));
    } else {
        puts("\nError, expected '(', unary operator, identifier, or literal");
        return -1;
    }

    assert(n != NULL);

    ast_node *ret = n; //This is the default if the epxression 
    //has already ended by this point. Usually ret will be 
    //overwritten by an expression that will use n as one of
    //its children

    //At this point, we are in the state E <- E.
    //and we need to use the lookahead to figure out what to do
    while (1) {
        if (state->lookahead.type == TOK_KW && IS_POSTFIX_KW(state->lookahead.as_kw)) {
            // E <- E postfix_op
            ret = ast_node_alloc(a);
            ret->node_type = POSTFIX_OP;
            ret->op = state->lookahead.as_kw;
            ast_node_set_child(a, ret, 0, n);
            OBTAINT(state, &(state->lookahead));
        } else if (state->lookahead.type == TOK_KW && IS_BOP_KW(state->lookahead.as_kw)) {
            // E <- E binary_op E, except we need to check precedence and 
            // associativity
            kw_t LA = state->lookahead.as_kw;
            if (
                bop_prec[LA] > bop_prec[last_bop] ||
                (bop_prec[LA] == bop_prec[last_bop] && bop_is_rassoc[LA])
            ) {
                // bop in lookahead must be performed first
                kw_t op = LA;
                ast_node *rhs; //n is the lhs
                OBTAINT(state, &(state->lookahead));
                int rc = parse_expr(state, op, a, &rhs);
                if (rc < 0) return rc; //Propagate error code
                ret = ast_node_alloc(a);
                ret->node_type = BINARY_OP;
                ret->op = op;
                ast_node_set_child(a, ret, 0, n);
                ast_node_set_child(a, ret, 1, rhs);
            } else {
                n = ret; //Very subtle
                break;
            }
        } else if (state->lookahead.type == TOK_KW && state->lookahead.as_kw == '[') {
            ast_node *index_expr;
            OBTAINT(state, &(state->lookahead));
            int rc = parse_expr(state, 0, a, &index_expr);
            if (rc < 0) return rc; //Propagate error code
            if (state->lookahead.type != TOK_KW || state->lookahead.as_kw != ']') {
                puts("\nError, unmatched '['");
                return -1;
            }
            
            //Advance past ']'
            OBTAINT(state, &(state->lookahead));

            ret = ast_node_alloc(a);
            ret->node_type = INDEX_EXPR;
            ast_node_set_child(a, ret, 0, n);
            ast_node_set_child(a, ret, 1, index_expr);
        } else if (
            (state->lookahead.type == TOK_KW && state->lookahead.as_kw == '?') && 
            (bop_prec[last_bop] <= bop_prec['='])
        ) {
            //n is the condition expression
            ast_node *val_if_true;
            OBTAINT(state, &(state->lookahead));
            int rc = parse_expr(state, 0, a, &val_if_true);
            if (rc < 0) return rc; //Propagate error code

            if (state->lookahead.type != TOK_KW || state->lookahead.as_kw != ':') {
                puts("\nError, '?' without ':'");
                return -1;
            }

            ast_node *val_if_false;
            OBTAINT(state, &(state->lookahead));
            rc = parse_expr(state, 0, a, &val_if_false);
            if (rc < 0) return rc; //Propagate error code

            ret = ast_node_alloc(a);
            ret->node_type = TERNARY_OP;
            ast_node_set_child(a, ret, 0, n);
            ast_node_set_child(a, ret, 1, val_if_true);
            ast_node_set_child(a, ret, 2, val_if_false);
        } else {
            n = ret; //Very subtle
            break;
        }

        n = ret; //Very subtle
    }

    *n_out = ret;
    return 0;
}