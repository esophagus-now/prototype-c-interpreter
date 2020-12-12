A few notes about this program's structure:

LEXER AND PARSER "OBJECTS"
--------------------------

First of all, I am a big fan of the coroutine idiom. Sadly, C does not have native coroutines, so I use an approximation. 

Notice that my lexer and parser functions all take a pointer to a lexer_state/parse_state. In essence, these are the coroutine's local variables, and you can maybe imagine that all the return statements are yield statements.

 -> Of course, this state struct feels a lot like a this pointer, doesn't it? You may find it easier to understand the lexer_state as an object with get_token as a member function (and likewise for parse_state and the parse_X functions)


GENERALIZED CHARACTER/TOKEN OBTAINMENT
--------------------------------------

Basically, instead of baking one particular method of getting characters in the lexer (e.g. calling getchar() or fgetchar()) the lexer_state/parse_state have function pointers. Like any good callback mechanism, the lexer_state/parse_state maintains a void* that is passed as argument to this obtainment function.

This is a great example of a decision that complicates the code for the sake of generality. Of course, you always have to ask yourself if excessive generality is worth the trouble, but in my case I think it is:

 - I want to be able to lex tokens from interactive typing
 - In the future, I want to be able to pass meta-characters that tell the lexer/parser to return predictions
 - I want to be ablt to lex tokens from files
 - I expect in the future to want to lex tokens from strings
 - Also, the generalization turned out to be unexpectedly handy for debugging
 - Very importantly, it is now possible to select a token from a list and give it to parsing functions, rather than having to read all the characters (in other words, I want it to be possible to click a suggestion and have that suggestion be put in automatically. The reason to avoid reading indiidual characters is not for performance, but rather because we don't want to complicate the code with extra suport for pushing fake characters into the stream that the lexer reads from)


 -> By the way, yes I am aware of fdopen and fmemopen, and by looking at the source code from fmemopen, you can easily see how you could slot your own function pointers into a FILE * struct. This might actually be a better solution, but at least for a prototype, I didn't want to go down that road.


LOOKAHEAD MANAGEMENT CONVENTION
-------------------------------
(Sounds like some BS training event for managers)

I have adopted a convention for dealing with lookahead. Basically, all lexing/parsing functions (with very few exceptions) all follow the same rules:
  1. Read the lookahead before reading from anywhere else
  2. Update the lookahead before returning

The problem is that the lookahead (which is stored in the lexer_state/parse_state) needs to be initialized before the first call to the lexing/parsing functions. This means that when you call init_lexer_state or init_parse_state (a.k.a. the constructor) this would result in a call to the character obtainment function. The issue is that the character obtainment call could be blocking, and you might end up with a deadlock scenario if the usre of the lexing function needs to do more setup before reading characters. My workaround is as follows:

  - The lexer start with a space character in its lookahead.
  - The parser has an extra function (kind of like a part-two of the constructor) to intialize the lookahead. However, I cleverly hid it behind a piece of functionality that you would need anyway: the peek_nonterm function. I don't have a top-level "parse my C file" function. Instead, you are supposed to call peek_nonterm to figure out which parsing function to call next (e.g. parse_stmt, parse_expr, parse_decl, etc.) So, all the parsing functions assume the lookahead is valid _except for peek_nonterm_, which bothers to check the lookahead_vld flag.




MISSING FEATURES
----------------

First of all, I was always intending to rewrite this code from scratch. Take a look at zz_fix_in_rewrite.txt where I've been keeping a list of improvements.




LIST OF REMAINING GOALS FOR THIS PROTOTYPE
------------------------------------------
  
[ ] Add at least basic support for the preprocessor
    -> Maybe just simple macros
[ ] Add evaluation function that walks an AST and runs the code.
[ ] Add support for initializers in declarations
[ ] Edit parse_decl to make use of the AST
[ ] Write the number parser
    -> Should this be done by the lexer? Or should the lexer just save the string and it will get parsed later on?
[ ] Extend dummy statement parser to support control-flow blocks
[ ] Support function declarations