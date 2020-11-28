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

void ast_init(ast *a);

void ast_deinit(ast *a);

ast *ast_new();

void ast_free(ast *a);

static void expand_ast_if_full(ast *a);

ast_node *ast_node_alloc(ast *a);

#define HANDLE_OF(a, x) ((x) - ((a)->storage))

void ast_node_free(ast *a, ast_node *n);

//Sets parent->children[num] to the given ast_node
void ast_node_set_child(ast *a, ast_node *parent, int num, ast_node *child);

ast_node *ast_node_get_child(ast *a, ast_node *parent, int num);

void print_ast(ast *a, ast_node *root);

#endif