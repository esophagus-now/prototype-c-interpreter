#include <stdio.h>
#include "lexer.h"
#include "parse_common.h"
#include "parse_expr.h"
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
    obtaint_fn *f = (obtaint_fn *) get_token;

    string_with_pos swp = {
        .str = "x[*p++] = (6+7)*8?c=3:5%6;",
        .pos = 0
    };

    lexer_state lstate;
    lexer_state_init(&lstate, getchar_dbg, NULL);
    //lexer_state_init(&lstate, my_obtainer, &swp);

    ast *a = ast_new();
    ast_node *root;

    parse_state pstate;
    parse_state_init(&pstate, (obtaint_fn *) get_token, &lstate);

    nonterm_t type = peek_nonterm(&pstate);
    if (type != NT_EXPR) {
        puts("Not an expression. Time to go home");
        ast_free(a);
        return 0;
    }

    int rc = parse_expr(&pstate, 0, a, &root);
    if (rc < 0) {
        puts("\nParsing error");
        ast_free(a);
        return -1;
    } else {
        print_ast(a, root);
    }

    ast_free(a);

    puts("\nDone");

    /*
    token t;

    while (1) {
        int rc = get_token(&t, &state);
        if (rc < 0) {
            printf("Error! Code = %d\n", rc);
            break;
        } else if (t.type == TOK_EOS) {
            puts("Done!");
            break;
        } else {
            printf("Token type: %s", token_t_names[t.type]);
            switch(t.type) {
            case TOK_IDENT:
                printf(", name = %s\n", t.as_ident.name);
                break;
            case TOK_NUMBER:
                printf(", (number parsing not implemented)\n");
                break;
            case TOK_STR:
                printf(", \"%s\"\n", t.as_str);
                break;
            case TOK_KW:
                if (t.as_kw < 256) {
                    printf(", %c\n", t.as_kw);
                } else {
                    printf(", %s\n", keywords[t.as_kw - 256]);
                }
                break;
            default:
                printf(", (unimplemented)\n");
                break;
            }
        }
    }
    */
    return 0;
}