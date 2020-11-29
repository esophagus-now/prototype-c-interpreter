/*
A type is represented as a string of simple
elements called "tq"s (short for "type quanta"). 
There are four basic types of tqs:

Builtin types or USER_TYPE:
    This stores a bitmask of qualifiers, i.e. static, 
    extern, register, auto, const, or volatile.
    
POINTER:
    In the string, a POINTER tq followed by another tq
    containing type x means "pointer-to-x". The tq also
    keeps track of qualifiers for the pointer itself.

ARRAY:
    In the string, an ARRAY tq followed by another tq
    containing type x means "array-of-x". The tq also
    keeps track of the length of the array.

FUNCTION:
    This tq indicates how many arguments the function
    has. The following items in the string of tqs are,
    in this order, all parameters (if any) then the 
    return type.

*/

#ifndef TYPE_H
#define TYPE_H 1

#include <stdint.h>

typedef enum {
    TQ_EOS, //Like a NUL character in strings. Technically,
            //you don't need this to see when a tq ends, because
            //it always ends on the first TQ_BASE_TYPE entry that
            //isn't a function return type or parameter. But that
            //kind of complicates program logic to have to deal 
            //with that all the time
    TQ_CHAR,
    TQ_SHORT,
    TQ_INT,
    TQ_LONG,
    TQ_LL, /*Currently unsupported*/
    TQ_FLOAT,
    TQ_DOUBLE,
    TQ_VOID,
    TQ_BITFIELD,
    TQ_POINTER,
    TQ_IDENT,
    TQ_USER_TYPE,
    TQ_FUNCTION,
    TQ_ARRAY,
    TQ_STRUCT,
} tq_t;

#define IS_BUILTIN(x) ((x) >= TQ_CHAR && (x) <= TQ_BITFIELD)
#define IS_BASE_TYPE(x) (IS_BUILTIN(x) || ((x) == TQ_USER_TYPE))

//TODO: should unsigned be a qualifier? What about long and short? long long?
#define TQ_CONST        (1<<0)
#define TQ_VOLATILE     (1<<1)
#define TQ_EXTERN       (1<<2)
#define TQ_STATIC       (1<<3)
#define TQ_REGISTER     (1<<4)
#define TQ_AUTO         (1<<5)
#define TQ_COMPLEX      (1<<6)
#define TQ_UNSIGNED     (1<<7)

#define ARRAY_AUTO_SIZE UINT32_MAX

typedef struct tq {
    tq_t type;
    union {
        struct {
            unsigned qualifiers; 
        } as_builtin;

        struct {
            unsigned qualifiers;
        } as_pointer;

        struct {
            char const *name;
            uint32_t hash;
        } as_ident;

        struct {
            struct tq *tstr;
        } as_user_type;

        struct {
            unsigned num_params; //Parameters follow in type string
            //For example, the following tstr would be generated for
            //void my_fn(int x, char const *dest):
            //TQ_IDENT, "my_fn"
            //TQ_FUNCTION, num_params = 2
            //TQ_IDENT, "x"
            //TQ_INT, qualifiers = 0
            //TQ_IDENT, "str"
            //TQ_POINTER, qualifiers = 0
            //TQ_INT, qualifiers = TQ_CONST
            //TQ_VOID
            //TQ_EOS
        } as_function;
        
        struct {
            //TODO: should I support larger sizes?
            unsigned array_len; //If equal to UINT32_MAX, then it means
            //the compiler should deduce the size from the initializer
        } as_array;

        struct {
            unsigned num_entries; //Entries follow in type string
        } as_struct;
    };
} tq;

//I always end up with a dilemma when I have to manage
//arbitrary-length strings. There are three main techniques
//for memory management:
// 1. Pre-allocate a fixed-size array that is "bigger than
//    the longest possible string".
// 1. (variation) Add bounds-checking so that we throw an
//    error instead of overflowing (like the famous gets() worm)
// 2. Use a heap-allocated array that we automatically
//    realloc when we're out of space (à la std::vector).
// 3. Custom pool allocator.
//
//Each has pros and cons. 1. is very simple and can be very 
//efficient if you have some gaurantee that it's impossible to
//overflow your fixed-buffer. The downside is that you could 
//potentially be wasting a lot of memory.
//2. is not that hard to implement, and makes efficient use of
//memory. It adds a performance overhead (and has more moving
//parts, which could introduce bugs). How much overhead? It
//depends on a lot of things and there's no point speculating.
//3. is the most tedious and error-prone to implement. It is
//unlikely that you would get better performance than malloc,
//anyway, and often implements 2. behind the scenes. The only 
//reason to do this would be to make it easier to serialize 
//(and even that is not always true).

//Incidentally, 2. can waste a lot of memory in the worst case.
//usually when you extend the capacity you double it, so the
//waste could actually be worse than 1.

//Same semantics as strlen (doesn't count terminator)
unsigned tqlen(tq const *t);

//Like strdup but for tqs
tq* tqdup(tq const *t);

//Returns position after last-printed item
tq const* dbg_print_tq(tq const *t);

#endif