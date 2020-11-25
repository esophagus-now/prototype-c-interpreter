#ifndef SYMTAB_H
#define SYMTAB_H 1

#include <stdint.h>

#include "../type.h"
#include "vector.h"

//A symbol is anything that can be indicated by
//an identifier in the C code we are parsing. This
//can be a variable, a function, or a user-defined 
//type name.
//
//This means we need a "reasonably efficient" (but
//most of all, simple!) way to map from strings to
//some symbol description struct. 

//A symbol might need any of the following information:
//
//symbol type: variable, function, or type name
//scope: where this symbol is variable
//data type: represented by a tq string

typedef enum {
    VARIABLE, //Also includes functions
    TYPENAME
} symbol_t;

typedef struct {
    symbol_t sym_type; //Normally I would call this "type", but
    //don't want to accidentally use this when I want the data
    //type (or vice-versa)

    uint32_t hash;
    char *name;

    //TODO: some way to indicate scope. Actually, I was
    //thinking that at any given moment when you're 
    //parsing code, you actually maintain a stack of
    //symbol tables and pop the top one off when you leave
    //a scope/translation unit.

    tq *data_type;
} symbol;

int symcmp(symbol const *a, symbol const *b);

typedef struct {
    VECTOR_DECL(symbol, data);
} symtab;

void symtab_init(symtab *s);

//Only frees contents of s, not s itself
void symtab_free(symtab *s);

//Stolen from c4. No idea where they got this from
static inline uint32_t step_hash(uint32_t prev, char c) {
    return prev*147 + c;
}

uint32_t hash_str(char *str);

#endif