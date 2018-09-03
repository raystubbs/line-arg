#ifndef lnA_line_arg_h
#define lnA_line_arg_h

#define lnA_MAX_DESC_WIDTH (70)

typedef struct lnA_Parser lnA_Parser;
typedef struct lnA_Usage  lnA_Usage;

typedef void
(*lnA_ParamCb)( char* arg, void* udata );

typedef void
(*lnA_OptionCb)( char* opt, void* udata );

lnA_Parser*
lnA_makeParser( char* name, void* udata );

void
lnA_freeParser( lnA_Parser* par );

lnA_Usage*
lnA_addUsage( lnA_Parser* par, char* usage );

void
lnA_addParam( lnA_Parser* par, char* name, lnA_ParamCb cb );

void
lnA_addOption( lnA_Parser* par, char* sf, char* lf, char* desc, lnA_OptionCb cb );

void
lnA_setHeader( lnA_Parser* par, char* header );

void
lnA_setFooter( lnA_Parser* par, char* footer );

// Prints the usage text generated from the
// combination of usage strings, option descriptions
// header text, and footer text.
void
lnA_printUsage( lnA_Parser* par );

// Tries to parse the given argument set with
// the specified usage, on success returns NULL,
// on failure returns a brief error message
char*
lnA_tryUsage( lnA_Parser* par, lnA_Usage* usg, char** argv );

#endif
