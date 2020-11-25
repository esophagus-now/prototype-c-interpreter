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