#include <stdio.h>
#include "lexer.h"
#include "parse_common.h"
#include "parse_expr.h"
#include "parse_type.h"
#include "parse_stmt.h"
#include "ast.h"

typedef struct {
    char *str;
    int pos;
} string_with_pos;

int my_obtainer(void *arg) {
    string_with_pos *swp = (string_with_pos *) arg;
    return swp->str[swp->pos++];
}

int getchar_dbg(void *arg) {
    int ret = getchar();
    if (arg != NULL) printf("You entered %d\n", ret);
    return ret;
}

int main(void) {
    puts("Keeping in mind that only a small subset of the language is supported,");
    puts("go ahead and start typing some C code. This program will build an AST");
    puts("for expressions and print out an in-order traveral with disamgibuating");
    puts("parentheses. For type declarations, a human-readable string will be");
    puts("printed. Use semicolons at the end of your expressions/types, or hit");
    puts("CTRL-D to input an EOF character (thus quitting the program)");
    obtaint_fn *f = (obtaint_fn *) get_token;


    //These structs are used if you want to use the my_obtiner function
    //to input characters. See lines 49-52; these are three different 
    //choices of input
    string_with_pos swp = {
        .str = "x[*p++] = (6+7)*8?c=3:5%6;",
        .pos = 0
    };

    string_with_pos swp2 = {
        .str = "int (*x)(char *, int) = NULL;",
        .pos = 0
    };

    lexer_state lstate;
    lexer_state_init(&lstate, getchar_dbg, NULL);
    //lexer_state_init(&lstate, my_obtainer, &swp);
    //lexer_state_init(&lstate, my_obtainer, &swp2);


    parse_state pstate;
    parse_state_init(&pstate, (obtaint_fn *) get_token, &lstate);

    //Main loop: ask what type of non-terminal is coming up next,
    //then parse it. For now, expressions are saved in an AST and
    //types are written in a tq string (see type.h). This loop
    //just prints them out.
    nonterm_t type;
    while ((type = peek_nonterm(&pstate)) != NT_EOS) {
        if (type == NT_EXPR) {
            ast *a = ast_new();
            ast_node *root;
            int rc = parse_expr(&pstate, 0, a, &root);
            if (rc < 0) {
                puts("\nParsing error");
                ast_free(a);
                return -1;
            } else {
                print_ast(a, root);
                ast_free(a);
            }
        } else if (type == NT_DECL) {
            VECTOR_DECL(tq, tstr);
            vector_init(tstr);

            int rc = parse_type(&pstate, VECTOR_ARG(tstr));
            if (rc < 0) {
                puts("\nCould not parse declaration");
                vector_free(tstr);
            } else {
                dbg_print_tq(tstr);
                vector_free(tstr);
            }
        } else if (type == NT_STMT) {
            //Right not, the statement parser is just a dummy.
            int rc = parse_stmt(&pstate);
            if (rc < 0) break;
            puts("\n(semicolon)");
        } else {
            puts("\nSorry, no support for this type of statement");
            break;
        }
    }


    puts("\nDone");
    return 0;
}