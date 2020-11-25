#include <string.h>
#include "symtab.h"
#include "vector.h"

int symcmp(symbol const *a, symbol const *b) {
    int ret = a->hash - b->hash;
    if (ret == 0) {
        return strcmp(a->name, b->name);
    }
    return ret;
}

void symtab_init(symtab *s) {
    vector_init(s->data);
}

//Only frees contents of s, not s itself
void symtab_free(symtab *s) {
    vector_free(s->data);
}

uint32_t hash_str(char *str) {
    uint32_t ret = *str++;

    char c;
    while((c = *str) != 0) ret = step_hash(ret, c);
    
    return ret;
}
