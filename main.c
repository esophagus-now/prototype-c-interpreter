#include <stdio.h>
#include "lexer.h"

int my_obtainer() {
    static char *str = "int x;";
    return *str++;
}

int getchar_dbg() {
    int ret = getchar();
    printf("You entered %d\n", ret);
    return ret;
}

int main(void) {
    lexer_state state;
    lexer_state_init(&state, getchar);

    token t;

    while (1) {
        int rc = get_token(&state, &t);
        if (rc < 0) {
            printf("Error! Code = %d\n", rc);
            break;
        } else if (rc == 0) {
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

    printf("\nHello World\n");
    return 0;
}