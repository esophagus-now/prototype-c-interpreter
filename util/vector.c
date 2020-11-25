#include <stdlib.h>
#include "vector.h"
#include "fast_fail.h"

void vector_extend(unsigned elem_sz, unsigned *cap, void **data) {
    *data = realloc(*data, *cap*2*elem_sz);
    if (!*data) FAST_FAIL("out of memory");
    *cap *= 2;
}

void __vector_init(unsigned elem_sz, unsigned *len, unsigned *cap, void **data) {
    *data = malloc(VECTOR_INIT_SZ*elem_sz);
    if (!*data) FAST_FAIL("out of memory");
    *len = 0;
    *cap = VECTOR_INIT_SZ;
}

//Could just call free directly, but this is more consistent
void vector_free(void *v) {
    free(v);
}

void* __vector_lengthen(unsigned elem_sz, unsigned *len, unsigned *cap, void **data) {
    if (*len == *cap) vector_extend(elem_sz, cap, data);

    return *data + elem_sz * (*len)++;
}

void __vector_shrink_to_fit(unsigned elem_sz, unsigned *len, unsigned *cap, void **v) {
    void *ret = realloc(*v, *len*elem_sz);
    if (!ret) FAST_FAIL("reallocation failed");
    *v = ret;
    *cap = *len;
}