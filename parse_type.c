#include <stdio.h>
#include "type.h"
#include "parse_type.h"
#include "util/vector.h"

int parse_type(char *str, VECTOR_PTR_PARAM(tq, tstr)) {
    int pos = 0;
    int stktop; //str will be split at pos, and everything
                //before pos is treated as a stack

    //TODO: incomplete, there are many other tokens that delimit the
    //end of a type decl
    while (str[pos] != 'n' && str[pos] != ')' && str[pos] != ',') {
        if (str[pos] == '\0') {
            puts("\nUnexpected EOS");
            return -1;
        }
        pos++;
    }
    
    stktop = pos - 1;

    //At this point pos indicates the ident/closing bracket
    if (str[pos] == 'n') {
        tq *elem = vector_lengthen(*tstr);
        elem->type = TQ_IDENT;
        printf("n is a");
        pos++;
    }

    while(1) {
        if (str[pos] == '[') {
            pos++;
            printf("n array of %c", str[pos++]); //One-character size
            tq *elem = vector_lengthen(*tstr);
            elem->type = TQ_ARRAY;
            unsigned len = strtoul(str, NULL, 0);
            if (str[pos++] != ']') {
                puts("\nError: unmatched `['");
                return -1;
            }
        } else if (str[pos] == '(') {
            pos++;
            printf(" function taking {");
            tq *fn = vector_lengthen(*tstr);
            fn->type = TQ_FUNCTION;
            int print_delim = 0;
            do {
                if (print_delim) printf(",");
                int incr = parse_type(str + pos, VECTOR_ARG(*tstr));
                if (incr < 0) return incr;
                pos += incr;
                print_delim = 1;
            } while (str[pos++] == ','); 
            //^Silently does not check if proper closing ')' is present
            fn->as_function.num_params = vector_back_ptr(*tstr) - fn; //Tricky dicky
            printf(" } and returning");
        } else if (stktop >= 0) {
            if (str[stktop] == '*') {
                printf(" pointer to");
                tq * elem = vector_lengthen(*tstr);
                elem->type = TQ_POINTER;
                elem->as_pointer.qualifiers = 0; //TODO: deal with qualifiers
                stktop--;
            } else if (str[stktop] == '(') {
                if (str[pos++] != ')') {
                    puts("\nError: unmatched '('");
                    return -1;
                }
                stktop--;
            } else {
                #warning NOT FINSIHED
                switch (str[stktop]) {
                case 'i':
                    printf(" int");
                    break;
                case 'v':
                    printf(" void");
                    break;
                case 'c':
                    printf(" char");
                    break;
                case 'f':
                    printf(" float");
                    break;
                default:
                    printf("\nError: unknown type `%c'\n", str[stktop]);
                    return -1;
                }
                stktop--;
            }
        } else break;
    }

    return pos; //TODO: return next parsing position
}