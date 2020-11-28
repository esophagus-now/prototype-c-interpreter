#ifndef AST_H
#define AST_H 1

//Look, I wrote all this code before I wrote vector.h
//Yeah, I might come back and fix up this code to use it,
//but don't hold your breath.

//By the way, this idea of having my own pool of nodes 
//in a vector was because I wanted easy serialization. 
//But looking back, that's a little misguided. It would
//have been much simpler to just use malloc/free for each
//node

#include <assert.h>
#include <stdlib.h>

#include "lexer.h"

//This got really messy on me. I'm now defining similar
//enum values in multiple different header files. Of course,
//my plan was always to completely rewrite everything from
//scratch once I had encountered all these types of issues
typedef enum {
    NUMBER,
    IDENT,
    UNARY_OP,
    POSTFIX_OP,
    BINARY_OP,
    INDEX_EXPR,
    TERNARY_OP
} astnode_t;

#define MAX_IDENT 32

typedef struct {
    astnode_t node_type;

    //TODO: has to be updated to use all my new data types
    union {
        kw_t op;
        long value;
        char ident_str[MAX_IDENT + 1];
    };

    union {
        int next; //Makes code nicer to read
        int children[3];
    };
} ast_node;

typedef struct {
    int tree_root; //Not currently used???

    int pos;
    int sz;
    ast_node *storage;
} ast;

#define AST_INIT_SZ 32
void ast_init(ast *a) {
    a->storage = malloc(AST_INIT_SZ * sizeof(ast_node));
    a->pos = 1; //Index zero is reserved
    a->tree_root = 0;
    a->sz = AST_INIT_SZ;

    //Index zero of storage is always a sentinel,
    //which also doubles as the head of the free list
    int i;
    for (i = 0; i < AST_INIT_SZ - 1; i++)
        a->storage[i].next = i + 1;
    
    //Circular linked list
    a->storage[AST_INIT_SZ - 1].next = 0; 
}

void ast_deinit(ast *a) {
    //Strinctly speaking resetting it all to zero
    //isn't necessary...
    a->tree_root = 0; 
    a->sz = 0;
    a->pos = 0;
    if (!a->storage) return;
    free(a->storage);
}

ast *ast_new() {
    //For simplicity: no error checking
    ast *ret = malloc(sizeof(ast));

    ast_init(ret);

    return ret;
}

void ast_free(ast *a) {
    if (!a) return;
    ast_deinit(a);
    free(a);
}

static void expand_ast_if_full(ast *a) {
    //a->storage[0] is reserved as the head of the linked list
    //of free nodes
    if (a->storage[0].next == 0 && a->pos == a->sz) {
        ast_node *bigger = realloc(a->storage, a->sz*2*sizeof(ast_node));
        //Really should do error checking...
        a->storage = bigger;

        //Free list now indicates first free node
        a->storage[0].next = a->sz;

        //Construct the linked list through all the other nodes
        int i;
        for (i = a->sz; i < a->sz*2 - 1; i++)
            a->storage[i].next = i + 1;

        //Circularly linked
        a->storage[a->sz*2 - 1].next = 0;

        a->sz *= 2;
    }
}

ast_node *ast_node_alloc(ast *a) {
    expand_ast_if_full(a);

    //Get the index of the next free node.
    int offset = a->storage[0].next;
    assert(offset != 0);

    //Move the head of the free list to point to the
    //first node after the free node.
    a->storage[0].next = a->storage[offset].next;

    return a->storage + offset;
}

#define HANDLE_OF(a, x) ((x) - ((a)->storage))

void ast_node_free(ast *a, ast_node *n) {
    int handle = HANDLE_OF(a, n);
    //Add this node back into the linked list
    a->storage[handle].next = a->storage[0].next;
    a->storage[0].next = handle;
}

//Sets parent->children[num] to the given ast_node
void ast_node_set_child(ast *a, ast_node *parent, int num, ast_node *child) {
    int offset = HANDLE_OF(a, child);
    parent->children[num] = offset;
} 

ast_node *ast_node_get_child(ast *a, ast_node *parent, int num) {
    return a->storage + parent->children[num];
}

void print_ast(ast *a, ast_node *root) {
    switch(root->node_type) {
    case NUMBER:
        printf("%ld", root->value);
        break;
    case IDENT:
        printf("%s", root->ident_str);
        break;
    case UNARY_OP:
        printf("%c(", root->op);
        print_ast(a, ast_node_get_child(a, root, 0));
        printf(")");
        break;
    case POSTFIX_OP:
        printf("(");
        print_ast(a, ast_node_get_child(a, root, 0));
        printf(")%c", root->op);
        break;
    case BINARY_OP:
        printf("(");
        print_ast(a, ast_node_get_child(a, root, 0));
        printf("%c", root->op);
        print_ast(a, ast_node_get_child(a, root, 1));
        printf(")");
        break;
    case INDEX_EXPR:
        printf("(");
        print_ast(a, ast_node_get_child(a, root, 0));
        printf(")[");
        print_ast(a, ast_node_get_child(a, root, 1));
        printf("]");
        break;
    case TERNARY_OP:
        printf("(");
        print_ast(a, ast_node_get_child(a, root, 0));
        printf(")?(");
        print_ast(a, ast_node_get_child(a, root, 1));
        printf("):(");
        print_ast(a, ast_node_get_child(a, root, 2));
        printf(")");
        break;
    default:
        assert(0 && "Unknown ast_node type");
        break;
    }
}


#endif