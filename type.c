#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"

//Same semantics as strlen (doesn't count terminator)
unsigned tqlen(tq const *t) {
    unsigned ret = 0;
    while((t++)->type != TQ_EOS) ret++;
    return ret;
}

//Like strdup but for tqs
tq* tqdup(tq const *t) {
    unsigned len = tqlen(t);

    tq *ret = malloc((len+1) * sizeof(tq));
    if (!ret) return ret;

    memcpy(ret, t, (len+1)*sizeof(tq));
    return ret;
}

//Returns position after last-printed item
tq const* dbg_print_tq(tq const *t) {
    if (t == NULL) return NULL;

    while (t->type != TQ_EOS) {
        //Could have used an X macro to auto-generate the strings,
        //but whatever
        switch(t->type) {
        case TQ_CHAR:
            printf("char");
            return t+1;
        case TQ_INT:
            printf("int");
            return t+1;
        case TQ_FLOAT:
            printf("float");
            return t+1;
        case TQ_DOUBLE:
            printf("double");
            return t+1;
        case TQ_VOID:
            printf("void");
            return t+1;
        case TQ_IDENT:
            printf("%s is a ", t->as_ident.name);
            t++;
            break;
        case TQ_POINTER:
            printf("pointer to ");
            t++;
            break;
        case TQ_ARRAY:
            if (t->as_array.array_len == ARRAY_AUTO_SIZE) {
                printf("auto-sized array of ");
            } else {
                //TODO: need proper number parsing in tokenizer
                printf("array of #NUM ");
            }
            t++;
            break;
        case TQ_FUNCTION: {
            printf("function taking ");
            if (t->as_function.num_params == 0) {
                printf("no arguments");
            } else {
                int i = t->as_function.num_params;
                t++;
                char const *delim = "";
                while (i --> 0) {
                    printf("%s", delim);
                    t = dbg_print_tq(t);
                    delim = ", ";
                }
            }

            printf(" and returning ");
            break;
        }
        default:
            puts("\nError (or just unsupported)");
            return NULL;
        }
    }

    return NULL;
}