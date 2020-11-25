#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"
#include "util/symtab.h"
#include "util/vector.h"

void lexer_state_init(lexer_state *l, obtainc_fn *fn) {
    l->lookahead = ' ';
    l->obtainc = fn;
}

//This array is used to map a keyword/operator string to a 
//Need to remember to add 256 to the index!
char const *const keywords[] = {
    #define X(x) #x 
    KEYWORDS
    #undef X

    #define X(name, str) str
    TWO_CHARACTER_OPERATORS
    #undef X
};

char const *const token_t_names[] = {
    #define X(x) #x
    TOKEN_TYPES
    #undef X
};

#define CLASSES \
    X(LOOK_AGAIN),\
    X(IDENT),\
    X(NUMBER),\
    X(WS),\
    X(PREPRO),\
    X(COMM),\
    X(KW),\
    X(CHAR),\
    X(STRING),\
    X(END_OF_INPUT),\
    X(ERROR)

typedef enum {
#define X(x) x
    CLASSES
#undef X
} class_t;

//Only useful for debug outputs. Not currently used
char const * const classnames[] = {
#define X(x) #x
    CLASSES
#undef X
};

static class_t classify(char c) {
    if (1 <= c && c <= ' ') return WS;
    if ('a' <= c && c <= 'z') return IDENT;
    if ('A' <= c && c <= 'Z') return IDENT;
    if ('0' <= c && c <= '9') return NUMBER;
    
    switch(c) {
    case '!': case '%': case '&': case '*': case '+': case '-':
    case '.': case '/': case '<': case '=': case '>': case '^':
    case '|': 
        return LOOK_AGAIN;

    case '(': case ')': case ',': case ':': case ';': case '?':
    case '[': case ']': case '{': case '}': case '~':
        return KW;

    case '"':
        return STRING;
    
    case '#':
        return PREPRO;

    case '\'':
        return CHAR;

    case '_':
        return IDENT;
    
    case '\0': case '\xFF':
        return END_OF_INPUT;

    default:
        return ERROR;
    }
}

//Returns 0 if not a keyword, otherwise, the correct
//keyword number. Only works for multi-char keywords!
static unsigned get_kw_number(char const *str) {
    unsigned i;
    for (i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return i + 256; //Need to add 256 to skip past single-character ops 
        }
    }
    return 0;
}

static void lex_ident(lexer_state *state, token *dest) {
    char c = state->lookahead;

    uint32_t hash = c;

    VECTOR_DECL(char, line);
    vector_init(line);

    vector_push(line, c);

    while(classify(c = state->obtainc()) == IDENT) {
        hash = step_hash(hash, c);
        vector_push(line, c);
    }

    vector_push(line, '\0');

    vector_shrink_to_fit(line);

    unsigned kw_num = get_kw_number(line);
    if (kw_num) {
        //printf("KW: [%s]\n", line);
        dest->type = TOK_KW;
        dest->as_kw = kw_num;
        state->lookahead = c;
        vector_free(line);
        return;
    }

    //printf("IDENT: [%s]\n", line);

    dest->type = TOK_IDENT;
    dest->as_ident.name = line; //Can't free line, we still need it!
    dest->as_ident.hash = hash;


    state->lookahead = c;
    return;
}

static void lex_single_char_kw(lexer_state *state, token *dest) {
    char c = state->lookahead;
    //printf("KW: [%c]\n", c);
    dest->type = TOK_KW;
    dest->as_kw = c;
    state->lookahead = state->obtainc();
}

//TODO: actually parse the number and support floating
//point types, deal with suffixes, etc. etc. etc.
//(ugh)
static void lex_num(lexer_state *state, token *dest) {
    char c = state->lookahead;

    VECTOR_DECL(char, line);
    vector_init(line);

    vector_push(line, c);
    while(classify(c = state->obtainc()) == NUMBER) vector_push(line, c);

    vector_push(line, '\0');

    //printf("NUM: [%s]\n", line);
    vector_free(line);

    dest->type = TOK_NUMBER;
    dest->as_number.type = TQ_UINT;
    dest->as_number.as_unsigned = 0xDEADBEEF; //TODO: parse number
    state->lookahead = c;
}

//Returns negative on error, one on sucess
static int lex_look_again(lexer_state *state, token *dest) {
    char pair[3];
    char first = state->lookahead;
    pair[0] = first;
    char second = state->obtainc();
    pair[1] = second;
    pair[2] = '\0';

    switch(first) {
    //Fall-through for the win!!!!
    case '-':
        if (second == '>') {
            //printf("KW: [%s]\n", pair);
            dest->type = TOK_KW;
            dest->as_kw = KW_arrow;
            state->lookahead = state->obtainc();

            return 0;
        }
        //Fall-through intended
    case '<': case '>': case '&': case '|': case '+': 
    case '=': 
        if (second == first) {
            //printf("KW: [%s]\n", pair);
            
            unsigned kw_num = get_kw_number(pair);
            assert(kw_num != 0);
            dest->type = TOK_KW;
            dest->as_kw = kw_num;

            state->lookahead = state->obtainc();
            return 1;
        }
        //Fall-through intended
    case '!': case '%': case '*': case '^': 
        if (second == '=') {
            //printf("KW: [%s]\n", pair);

            unsigned kw_num = get_kw_number(pair);
            assert(kw_num != 0);
            dest->type = TOK_KW;
            dest->as_kw = kw_num;

            state->lookahead = state->obtainc();
            return 1;
        } else {
            //printf("KW: [%c]\n", first);

            dest->type = TOK_KW;
            dest->as_kw = first; //ASCII value used as keyword number for single-char ops

            state->lookahead = second;
            return 1;
        }
        //Fall-through not possible
    case '/':
        if (second == '/') {
            char c;
            while ((c = state->obtainc()) != '\n');
            state->lookahead = c;
            return 1;
        } else if (second == '*') {
            puts("Block comments unsupported");
            return -1;
        } else if (second == '=') {
            //printf("KW: [%s]\n", pair);

            dest->type = TOK_KW;
            dest->as_kw = KW_div_eq;

            state->lookahead = state->obtainc();
            return 1;
        }
        //Fall-through intended
    default:
        //TODO: handle floats that start with a '.'
        //printf("KW: [%c]\n", first);            
        
        dest->type = TOK_KW;
        dest->as_kw = first; //ASCII value used as keyword number for single-char ops

        state->lookahead = second;
        return 1;
    }
}

static int hex_digit_to_num(char c, int *rc) {
    *rc = 0;
    if ('0' <= c && c <= '9') return (c - '0');
    else if ('A' <= c && c <= 'F') return (c + 10 - 'A');
    else if ('a' <= c && c <= 'f') return (c + 10 - 'a');
    else {
        *rc = 1;
        return 0;
    }
}

//Returns 0 on success, negative on error
//Places the escaped character into tmp, and advances 
//state->lookahead to the character after the escape sequence
static int resolve_escape(lexer_state *state, char *dest) {
    char c = state->lookahead;

    switch (c) {
    case '\\': *dest = '\\'; state->lookahead = state->obtainc(); break;
    case '\'': *dest = '\''; state->lookahead = state->obtainc(); break;
    case 't': *dest = '\t'; state->lookahead = state->obtainc(); break;
    case 'n': *dest = '\n'; state->lookahead = state->obtainc(); break;
    case 'v': *dest = '\v'; state->lookahead = state->obtainc(); break;
    case 'a': *dest = '\a'; state->lookahead = state->obtainc(); break;
    case '0': *dest = '\0'; state->lookahead = state->obtainc(); break;
    case '?': *dest = '\?'; state->lookahead = state->obtainc(); break;
    case '"': *dest = '"'; state->lookahead = state->obtainc(); break;
    case 'e': *dest = '\x1b'; state->lookahead = state->obtainc(); break;
    case 'x': {
        int rc;
        char tmp = hex_digit_to_num(state->obtainc(), &rc) << 4;
        if (rc != 0) {
            puts("Bad hex escape char");
            return rc; //Propagate error code
        }
        tmp |= hex_digit_to_num(state->obtainc(), &rc);
        if (rc != 0) {
            puts("Bad hex escape char");
            return rc; //Propagate error code
        }
        *dest = tmp;
        state->lookahead = state->obtainc();
        break;
    }
    //TODO: \u, \0NN, and probably a few rare cases I forgot
    default:
        printf("Unknown escape [\\%c]\n", c);
        state->lookahead = state->obtainc();
        return -1;
    }

    return 0;
}

//Returns 1 on success, negative on error
static int lex_char(lexer_state *state, token *dest) {
    char c = state->lookahead;
    char literal;

    if (c == '\\') {
        state->lookahead = state->obtainc();
        int rc = resolve_escape(state, &literal);
        if (rc != 0) {
            return rc; //TODO: use mm_err?
        }
    } else {
        literal = c;
        state->lookahead = state->obtainc();
    }

    if (state->lookahead != '\'') {
        puts("Error: multi-byte characters unsupported (did you forget a closing quote?)");
        return -1;
    }

    //printf("CHAR: [%c]\n", literal);

    dest->type = TOK_NUMBER;
    dest->as_number.type = TQ_CHAR;
    dest->as_number.as_unsigned = literal;

    state->lookahead = state->obtainc();
    return 1;
}

//Returns 1 on success, negative on error
static int lex_string(lexer_state *state, token *dest) {
    VECTOR_DECL(char, line);
    vector_init(line);

    while (state->lookahead != '"') {
        if (state->lookahead == '\\') {
            char tmp;
            int rc = resolve_escape(state, &tmp);
            if (rc != 0) {
                return rc; //Propagate error code
            }
            vector_push(line, tmp);
        } else if (state->lookahead == '\0') {
            puts("Unexpected EOF while parsing string (use \\0 instead)");
            return -1; //Propagate error code
        } else {
            vector_push(line, state->lookahead);
            state->lookahead = state->obtainc();
        }
    }

    vector_push(line, '\0');
    vector_shrink_to_fit(line);

    //printf("STRING: [%s]\n", line);

    dest->type = TOK_STR;
    dest->as_str = line; //Can't free line!

    state->lookahead = state->obtainc(); //Read past closing quote
    return 1;
}

static void lex_prepro(lexer_state *state, token *dest) {
    puts("Warning: preprocessor directives ignored");
    char c = state->lookahead;
    while (c != '\n') {
        if (c == '\\') {
            c = state->obtainc();
            if (c == '\n') c = state->obtainc();
        } else {
            c = state->obtainc();
        }
    }

    dest->type = TOK_ERROR;
    dest->as_error = "Preprocessor unsupported";

    state->lookahead = state->obtainc();
}

//Returns number of tokens read (which can never exceed one). If 
//it returns zero, then we have reached the end of the input.
//Negative return means error
int get_token(lexer_state *state, token *dest) {
    int ret = 1;

    class_t cls;
    while ((cls = classify(state->lookahead)) == WS) {
        state->lookahead = state->obtainc();
    }

    switch(cls) {
    case END_OF_INPUT:
        puts("Done");
        return 0;
    case IDENT:
        lex_ident(state, dest);
        break;
    case WS:
        ret = -1;
        break;
    case KW:
        lex_single_char_kw(state, dest);
        break;
    case NUMBER:
        lex_num(state, dest);
        break;
    case LOOK_AGAIN:
        ret = lex_look_again(state, dest);
        break;
    case CHAR:
        state->lookahead = state->obtainc(); //Read past opening quote
        ret = lex_char(state, dest);
        break;
    case STRING:
        state->lookahead = state->obtainc(); //Read past opening quote
        ret = lex_string(state, dest);
        break;
    case PREPRO:
        state->lookahead = state->obtainc(); //Read past pound character
        lex_prepro(state, dest);
        break;
    default:
        //TODO: the other classes
        ret = -1;
        break;
    }

    if (ret < 0) {
        puts("Lexing error");
    }
    return ret;
}