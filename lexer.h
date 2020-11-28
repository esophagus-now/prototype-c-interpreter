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

//TODO: move token type enums/macros into a separate header

//getc was already taken
typedef int obtainc_fn(void *arg);

typedef struct {
    char lookahead;
    obtainc_fn *obtainc;
    void *obtainc_arg; //Too slow?

    //TODO: symbol table, active macros table, etc.
} lexer_state;

void lexer_state_init(lexer_state *l, obtainc_fn *fn, void *obtainc_arg);

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

#define MULTI_CHARACTER_OPERATORS\
    X(plus_plus, "++"),\
    X(minus_minus, "--"),\
    X(neq, "!="),\
    X(mod_eq, "%="),\
    X(and_eq, "&="),\
    X(times_eq, "*="),\
    X(plus_eq, "+="),\
    X(arrow, "->"),\
    X(minus_eq, "-="),\
    X(div_eq, "/="),\
    X(lshift, "<<"),\
    X(leq, "<="),\
    X(eq_eq, "=="),\
    X(rshift, ">>"),\
    X(geq, ">="),\
    X(xor_eq, "^="),\
    X(or_eq, "|="),\
    X(lshift_eq, "<<="),\
    X(rshift_eq, ">>="),\
    X(and_and, "&&"),\
    X(or_or, "||"),\


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
    MULTI_CHARACTER_OPERATORS
    #undef X
} kw_t;

extern char const *const keywords[];

//These all have to be managed by hand if (for whatever reason)
//we decide to change the keywords (or their order)
#define IS_TYPE_DECL_KW(kw) (((kw)>=KW_char) && ((kw)<=KW_inline))
//FIXME: some of these are actually more like "labels" (case, default)
//and some of them could not be used by parse_stmt (else)
#define IS_STMT_KW(kw) (((kw)>=KW_if) && ((kw)<=KW_goto))
//I made a small effort to order these from most to least common
#define IS_UOP_KW(kw) (         \
    ((kw) == '*') ||            \
    ((kw) == '&') ||            \
    ((kw) == '!') ||            \
    ((kw) == KW_plus_plus) ||   \
    ((kw) == KW_minus_minus) || \
    ((kw) == '~') ||            \
    ((kw) == '-') ||            \
    ((kw) == '+')               \
)
#define IS_POSTFIX_KW(kw) (     \
    ((kw) == KW_plus_plus) ||   \
    ((kw) == KW_minus_minus)    \
)
//I made a small effort to order these from most to least common
#define IS_BOP_KW(kw) (         \
    ((kw) >= KW_neq) ||         \
    ((kw) == '=') ||            \
    ((kw) == '+') ||            \
    ((kw) == '-') ||            \
    ((kw) == '*') ||            \
    ((kw) == '/') ||            \
    ((kw) == '<') ||            \
    ((kw) == '>') ||            \
    ((kw) == '%') ||            \
    ((kw) == '&') ||            \
    ((kw) == '|') ||            \
    ((kw) == '^') ||            \
    ((kw) == ',')               \
)

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
//NOTE: these areguments are reversed from my usual style, but I 
//did this so get_token could be cast to an obtaint_fn type (see 
//parse_common.h)
int get_token(token *dest, lexer_state *state);

char const* dbg_keyword_as_str(kw_t num);

#endif