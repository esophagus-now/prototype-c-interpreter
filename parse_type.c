#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "type.h"
#include "parse_common.h"
#include "parse_type.h"
#include "util/vector.h"
#include "lexer.h"

//The method used in parse_type is more or less lifted from Deep
//C Secrets, and it requires you to first fill a stack up until
//a certain point. This helper funtion returns a truthy value if
//the given token delineates that "certain point"
static int should_stop_filling_stack(token const* t) {
    if (t->type == TOK_KW) {
        //I'm assuming the copmiler will be able to optimize this 
        //(Of course, even if it doesn't, no one will notice)
        if (IS_TYPE_DECL_KW(t->as_kw)) return 0;
        else if (t->as_kw == '*') return 0;
        else if (t->as_kw == '(') return 0;
    }

    return 1;
}

#define OBTAINT(state, tok) (state->obtaint(tok, state->obtaint_arg))
#define POP_TOKEN(state) OBTAINT(state, &state->lookahead)

//TODO: should this handle typedefs, or should I have a separate
//function for that?
//TODO: structs, unions, enums
//TODO: write another function to parse initializers and compound
//literals
//TODO: qualifiers
//(this is unending...)
//Returs 0 on success, negative otherwise
int parse_type(parse_state *state, VECTOR_PTR_PARAM(tq, tstr)) {
    VECTOR_DECL(token, tstack);
    vector_init(tstack);

    assert(state->lookahead_vld);

    //TODO: incomplete, there are many other tokens that delimit the
    //end of a type decl
    while (!should_stop_filling_stack(&state->lookahead)) {
        if (state->lookahead.type == TOK_EOS) {
            puts("\nUnexpected EOS");
            return -1;
        }
        
        //Push this token to the stack
        token *to_fill = vector_lengthen(tstack);
        memcpy(to_fill, &state->lookahead, sizeof(token));

        POP_TOKEN(state);
    }

    //At this point, we are ready to run the algorithm, but there is a
    //special case here to check for an identifier
    if (state->lookahead.type == TOK_IDENT) {
        tq *elem = vector_lengthen(*tstr);
        elem->type = TQ_IDENT;
        elem->as_ident.hash = state->lookahead.as_ident.hash;
        elem->as_ident.name = state->lookahead.as_ident.name;

        //Mark the name inside the token as NULL so that we don't 
        //double-free it 
        state->lookahead.as_ident.name = NULL;

        POP_TOKEN(state);
    }

    while(1) {
        if (state->lookahead.type == TOK_KW && state->lookahead.as_kw == '[') {
            POP_TOKEN(state);

            //TODO: allow compile-time expressions as array size
            tq *elem = vector_lengthen(*tstr);
            elem->type = TQ_ARRAY;

            if (state->lookahead.type == TOK_NUMBER) {
                //TODO: sanity-check the array size
                elem->as_array.array_len = state->lookahead.as_number.as_unsigned;
                POP_TOKEN(state);
            } else {
                elem->as_array.array_len = ARRAY_AUTO_SIZE;
            }

            if (state->lookahead.type != TOK_KW || state->lookahead.as_kw != ']') {
                puts("\nError: unmatched `['");
                return -1;
            }
            POP_TOKEN(state);
        } else if (state->lookahead.type == TOK_KW && state->lookahead.as_kw == '(') {
            tq *fn = vector_lengthen(*tstr);
            fn->type = TQ_FUNCTION;
            fn->as_function.num_params = 0;
            //Special case: C allows functions with no arguments to omit
            //the void keyword in the parameter list (e.g. int my_fun() {...} 
            //instead of int my_fun(void)). The least inelegant way to add 
            //support for this was to see if the recursive call to parse_type
            //added anything to the vector
            do {
                POP_TOKEN(state); //On first iter, pops '(', but pops ',' on later iters
                unsigned len_before = *tstr_len;
                int rc = parse_type(state, VECTOR_ARG(*tstr));
                if (rc < 0) return rc; //Propagate error code

                //If not for the special case mentioned above, we could have just
                //unconditionally incremented num_params
                if (*tstr_len > len_before) fn->as_function.num_params++;
            } while (state->lookahead.type == TOK_KW && state->lookahead.as_kw == ',');   

            if (state->lookahead.type != TOK_KW || state->lookahead.as_kw != ')') {
                puts("\nError: unmatched `('");
                return -1;
            }
        } else if (tstack_len > 0) {
            assert(vector_back_ptr(tstack)->type == TOK_KW);

            //TODO: qualifiers

            //The cast to unsigned gets rid of the compiler warning about 
            //'*' and '(' not belonging to the keyword enum
            switch ((unsigned) vector_back_ptr(tstack)->as_kw) {
            case '*': {
                tq * elem = vector_lengthen(*tstr);
                elem->type = TQ_POINTER;
                elem->as_pointer.qualifiers = 0; //TODO: deal with qualifiers
                break;
            }
            case '(':
                if (state->lookahead.type != TOK_KW || state->lookahead.as_kw != ')') {
                    puts("\nError: unmatched '('");
                    return -1;
                }
                POP_TOKEN(state);
                break;
            
            //I should really try to unify my enums so that I don't have to 
            //convert KW_(base type) to TQ_(base type)
            case KW_int: {
                tq * elem = vector_lengthen(*tstr);
                elem->type = TQ_INT;
                elem->as_builtin.qualifiers = 0; //TODO: deal with qualifiers
                break;
            }
            case KW_void: {
                tq * elem = vector_lengthen(*tstr);
                elem->type = TQ_VOID;
                elem->as_builtin.qualifiers = 0; //TODO: deal with qualifiers
                break;
            }
            case KW_char: {
                tq * elem = vector_lengthen(*tstr);
                elem->type = TQ_CHAR;
                elem->as_builtin.qualifiers = 0; //TODO: deal with qualifiers
                break;
            }
            case KW_float: {
                tq * elem = vector_lengthen(*tstr);
                elem->type = TQ_FLOAT;
                elem->as_builtin.qualifiers = 0; //TODO: deal with qualifiers
                break;
            }
            case KW_double: {
                tq * elem = vector_lengthen(*tstr);
                elem->type = TQ_DOUBLE;
                elem->as_builtin.qualifiers = 0; //TODO: deal with qualifiers
                break;
            }
            default:
                printf("\nError: unknown type `%s'\n", 
                    dbg_keyword_as_str(vector_back_ptr(tstack)->as_kw)
                );
                return -1;
            }
            vector_pop(tstack);
        } else break;
    }

    //This is something you can't do with C++ vectors:
    //Write a TQ_EOS at the end of the vector, but don't lengthen 
    //it. Makes sure that our tq string is always terminated
    vector_extend_if_full(*tstr);
    (*tstr)[*tstr_len].type = TQ_EOS;

    return 0;
}