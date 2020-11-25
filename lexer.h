/*
General idea: a lexer_t struct maintains the internal
state of the lexer. This struct also contains a pointer
to a function that returns the next char of input (is
this too slow?), where a zero char is understood as the
end of the stream.

We also need to implement the "lexer hack". The lexer 
needs to know about typedef if it is to correctly classify
the tokens as a type. This is why lexer_t contains a

*/

#ifndef LEXER_H
#define LEXER_H 1

#include <stdint.h>

#include "type.h"
#include "util/symtab.h"
#include "util/vector.h"

//getc was already taken
typedef int obtainc_fn();

typedef struct {
    char lookahead;
    obtainc_fn *obtainc;

    //TODO: symbol table, active macros table, etc.
} lexer_state;

void lexer_state_init(lexer_state *l, obtainc_fn *fn);

#define TOKEN_TYPES \
    X(TOK_IDENT), \
    X(TOK_NUMBER), \
    X(TOK_STR), \
    X(TOK_TYPE_NAME), \
    X(TOK_KW), \
    X(TOK_EOS), \
    X(TOK_ERROR),

typedef enum {
    #define X(x) x
    TOKEN_TYPES
    #undef X
} token_t;

//For debug outputs
extern char const *const token_t_names[];

//I count both keywords (e.g., int, for, return) and operators 
//(e.g., +, +=, ->) as keywords, and each one is given a number.
//The single-character operators have the same number as their
//ASCII representation. All other operators are assigned some
//number greater than 255.

#define KEYWORDS \
    X(char),	\
    X(short),	\
    X(int),	\
    X(long),	\
    X(float),	\
    X(double),	\
    X(void),	\
    X(struct),	\
    X(union),	\
    X(enum),	\
    X(typedef),	\
    X(unsigned),	\
    X(signed),	\
    X(const),	\
    X(volatile),	\
    X(static),	\
    X(extern),	\
    X(auto),	\
    X(register),	\
    X(inline),	\
    X(if),	\
    X(else),	\
    X(for),	\
    X(do),	\
    X(while),	\
    X(switch),	\
    X(case),	\
    X(default),	\
    X(break),	\
    X(continue),	\
    X(return),	\
    X(goto),

#define TWO_CHARACTER_OPERATORS\
    X(neq, "!="),\
    X(mod_eq, "%="),\
    X(and_eq, "&="),\
    X(times_eq, "*="),\
    X(plus_plus, "++"),\
    X(plus_eq, "+="),\
    X(minus_minus, "--"),\
    X(arrow, "->"),\
    X(minus_eq, "-="),\
    X(div_eq, "/="),\
    X(lshift, "<<"),\
    X(leq, "<="),\
    X(eq_eq, "=="),\
    X(rshift, ">>"),\
    X(geq, ">="),\
    X(xor_eq, "^="),\
    X(or_eq, "|="),


//This enum is what gives symbolic names to keywords
//numbered above 255. 
typedef enum {
    //This tricky dicky line makes all my keywords have 
    //numbers greater than 255
    __LAST_VALID_ASCII_CHAR = 255,
    
    #define X(x) KW_##x
    KEYWORDS
    #undef X

    //Keywords for the multi-character operators
    #define X(name, str) KW_##name
    TWO_CHARACTER_OPERATORS
    #undef X
} kw_t;

extern char const *const keywords[];

typedef struct {
    token_t type;
    union {
        struct {
            uint32_t hash;
            char *name;
        } as_ident;

        struct {
            tq_t type; //One of TQ_CHAR, TQ_UCHAR, TQ_SHORT, ... etc.
            union {
                //The interpreter should allow you to select the 
                //sizes of short, int, and long for each function
                //to get maximum portability. As such, we need to
                //store literals in the largest possible type, and
                //we can use narrowing conversions later on.
                unsigned long long as_unsigned;
                long long as_signed;
                //For floating-point
                float as_float;
                double as_double;
            };
        } as_number;

        char const *as_str;

        symbol as_typename; //Symbol contains name and hash, as well as data type

        kw_t as_kw;

        char const *as_error; //Keep this?
    };
} token;

//Returns number of tokens read (which can never exceed one). If 
//it returns zero, then we have reached the end of the input.
//Negative return means error
int get_token(lexer_state *state, token *dest);

#endif