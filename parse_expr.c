#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include "parse_expr.h"
#include "ast.h"
#include "lexer.h"

//TODO: there are many, many more operators and
//precedence levels. 
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

int bop_prec[] = {
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

int bop_is_rassoc[] = {
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

#if 0

//Returns 0 on success, or negative on error
int parse_expr(parse_state *state, char last_bop, ast *a, ast_node **n_out) {
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
        //E <- UE
        kw_t op = state->lookahead.as_kw;
        ast_node *operand_node;
        OBTAINT(state, &(state->lookahead));
        int rc = parse_expr(state, 0, a, &operand_node);
        if (rc < 0) return rc; //Propagate error code
        printf("(%c)", op); //Make sure to mark as unary op in output

        n = ast_node_alloc(a);
        n->node_type = UNARY_OP;
        n->op = op;
        ast_node_set_child(a, n, 0, operand_node);
    } else if (state->lookahead.type == TOK_IDENT) {
        // E <- ident 
        //TODO: struct member access
        n = ast_node_alloc(a);
        n->node_type = IDENT;
        char *dest = n->ident_str;
        printf("{");
        do {
            *dest++ = *str;
            printf("%c", *str++);
        } while (isalpha(*str));
        *dest = '\0';
        printf("}");
    } else if (isdigit(*str)) {
        //Quick-n-dirty
        n = ast_node_alloc(a);
        n->node_type = NUMBER;
        n->value = strtol(str, NULL, 0);
        printf("@");
        do {
            printf("%c", *str++);
        } while (isxdigit(*str));
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
    //and we need one token of lookahed to figure out what to do
    while (1) {
        char LA = *str;
        if (LA == '#') {
            //Postfix operator
            printf("#");
            str++;

            ret = ast_node_alloc(a);
            ret->node_type = POSTFIX_OP;
            ret->op = '#';
            ast_node_set_child(a, ret, 0, n);
        } else if (is_op(LA)) {
            if (
                bop_prec[LA] > bop_prec[last_bop] ||
                (bop_prec[LA] == bop_prec[last_bop] && bop_is_rassoc[LA])
            ) {
                // bop in lookahead must be performed first
                char op = *str++;
                ast_node *rhs; //n is the lhs
                int incr = parse_expr(str, op, a, &rhs);
                if (incr < 0) return incr; //Propagate error code
                str += incr;
                printf("%c", op);
                ret = ast_node_alloc(a);
                ret->node_type = BINARY_OP;
                ret->op = op;
                ast_node_set_child(a, ret, 0, n);
                ast_node_set_child(a, ret, 1, rhs);
            } else {
                n = ret; //Very subtle
                break;
            }
        } else if (LA == '[') {
            ast_node *index_expr;
            int incr = parse_expr(++str, 0, a, &index_expr);
            if (incr < 0) return incr; //Propagate error code
            str += incr;
            if (*str++ != ']') {
                puts("\nError, unmatched '['");
                return -1;
            }
            printf("["); //In postfix, we read this as an operator
            // that pops the top two things from the stack, and
            // indexes the first with the second

            ret = ast_node_alloc(a);
            ret->node_type = INDEX_EXPR;
            ast_node_set_child(a, ret, 0, n);
            ast_node_set_child(a, ret, 1, index_expr);
        } else if (LA == '?' && bop_prec[last_bop] <= bop_prec['=']) {
            //n is the condition expression
            ast_node *val_if_true;
            int incr = parse_expr(++str, 0, a, &val_if_true);
            if (incr < 0) return incr; //Propagate error code
            str += incr;
            if (*str++ != ':') {
                puts("\nError, '?' without ':'");
                return -1;
            }
            ast_node *val_if_false;
            incr = parse_expr(str, 0, a, &val_if_false);
            if (incr < 0) return incr; //Propagate error code
            str += incr;
            printf("?"); //In postfix, we read this as an operator
            // that pops the top three things from the stack, and
            // returns the second if the first is true, otherwise
            // the third.

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
    return str - str_saved;
}

int main(void) {
    //I am using '#' as a one-character replacement for '++'
    ast a;
    ast_init(&a);

    char const *test = "x[*p#]=2*(i-1)*r[i]?3*r[j-1]:43*mything;";
    printf("Original: %s\n", test);
    ast_node *test_root;
    printf("Postfix: ");
    int rc = parse_expr(test, 0, &a, &test_root);
    puts("");
    if (rc < 0) {
        puts("Failed");
        return -1;
    }

    printf("In-order AST traversal: ");
    print_ast(&a, test_root);
    puts("\n");

    char const *test2 = "x=y=1+2+3;";
    printf("Original: %s\n", test2);
    ast_deinit(&a);
    ast_init(&a);
    ast_node *test2_root;
    printf("Postfix: ");
    rc = parse_expr(test2, 0, &a, &test2_root);
    puts("");
    if (rc < 0) {
        puts("Failed");
        return -1;
    }

    printf("In-order AST traversal: ");
    print_ast(&a, test2_root);
    puts("");

    ast_deinit(&a);
    return 0;
}

#endif