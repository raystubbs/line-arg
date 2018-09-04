#include "line-arg.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct lnA_Usage {
    char*  usage;
    struct lnA_Usage* next;
} lnA_Usage;

typedef struct lnA_Param {
    char*       name;
    lnA_ParamCb callback;
    struct lnA_Param* next;
} lnA_Param;

typedef struct lnA_Option {
    char*        sForm;  // Short form (i.e -h)
    char*        lForm;  // Long form (i.e --help)
    char*        desc;   // Description text
    lnA_OptionCb callback;
    struct lnA_Option* next;
} lnA_Option;

typedef struct lnA_Queued {
    void
    (*callback)( char* str, void* udata );
    
    char* str;
    
    struct lnA_Queued* next;
} lnA_Queued;

typedef struct lnA_Queue {
    lnA_Queued* first;
    lnA_Queued* last;
} lnA_Queue;

typedef struct lnA_Parser {
    lnA_Usage*  uList;   // List of usage altenratives
    lnA_Param*  pList;   // List of parameter callbacks
    lnA_Option* oList;   // List of option descriptions
    char*       eText;   // Error message
    char*       pName;   // Program name
    char*       hText;   // Header text provided by user
    char*       fText;   // Footer text provided by user
    
    lnA_Queue*   qNow;   // Current callback queue
    lnA_Usage*   uNow;   // Current usage being parsed
    unsigned     uIdx;   // Index into usage string
    char**       argv;   // Current argument list
    unsigned     aIdx;   // Index into argument list
    
    void*        udata;  // User data passed to callbacks.
} lnA_Parser;

lnA_Parser*
lnA_makeParser( char* name, void* udata ) {
    lnA_Parser* par = malloc( sizeof(lnA_Parser) );
    *par = (lnA_Parser){ 0 };
    par->pName = name;
    par->udata = udata;
    return par;
}

void
lnA_freeParser( lnA_Parser* par ) {
    lnA_Usage* uIt = par->uList;
    while( uIt ) {
        lnA_Usage* tmp = uIt;
        uIt = uIt->next;
        free( tmp );
    }
    
    lnA_Option* oIt = par->oList;
    while( oIt ) {
        lnA_Option* tmp = oIt;
        oIt = oIt->next;
        free( tmp );    
    }
    
    lnA_Param* pIt = par->pList;
    while( pIt ) {
        lnA_Param* tmp = pIt;
        pIt = pIt->next;
        free( tmp );
    }
    
    if( par->eText )
        free( par->eText );
    
    free( par );
}

lnA_Usage*
lnA_addUsage( lnA_Parser* par, char* usage ) {
    lnA_Usage* usg = malloc(sizeof(lnA_Usage));
    usg->usage = usage;
    usg->next  = par->uList;
    par->uList = usg;
    return usg;
}

void
lnA_addParam( lnA_Parser* par, char* name, lnA_ParamCb cb ) {
    lnA_Param* prm = malloc(sizeof(lnA_Param));
    prm->name = name;
    prm->callback = cb;
    prm->next = par->pList;
    par->pList = prm;
}

void
lnA_addOption( lnA_Parser* par, char* sf, char* lf, char* desc, lnA_OptionCb cb ) {
    lnA_Option* opt = malloc(sizeof(lnA_Option));
    opt->sForm    = sf;
    opt->lForm    = lf;
    opt->desc     = desc;
    opt->callback = cb;
    opt->next  = par->oList;
    par->oList = opt;
}

void
lnA_setHeader( lnA_Parser* par, char* header ) {
    par->hText = header;
}

void
lnA_setFooter( lnA_Parser* par, char* footer ) {
    par->fText = footer;
}

void
lnA_printUsage( lnA_Parser* par ) {
    printf( "Usage:\n" );
    lnA_Usage* uIt = par->uList;
    while( uIt ) {
        printf( "  %s %s\n", par->pName, uIt->usage );
        uIt = uIt->next;
    }
    
    putchar( '\n' );
    
    if( par->hText )
        printf( "%s\n\n", par->hText );
        
    
    printf( "Options:\n" );
    lnA_Option* oIt = par->oList;
    while( oIt ) {
        if( oIt->sForm && oIt->lForm )
            printf( "-%s, --%s\n", oIt->sForm, oIt->lForm );
        else
        if( oIt->sForm )
            printf( "-%s\n", oIt->sForm );
        else
        if( oIt->lForm )
            printf( "--%s\n", oIt->lForm );
        
        unsigned len = strlen( oIt->desc );
        unsigned idx = 0;
        while( idx < len ) {
            printf( "  %.*s", lnA_MAX_DESC_WIDTH, &oIt->desc[idx] );
            idx += lnA_MAX_DESC_WIDTH;
        }
        printf( "\n\n" );
        oIt = oIt->next;
    }
    
    if( par->fText )
        printf( "%s\n\n", par->fText );
}

static char*
parseUsage( lnA_Parser* par );

char*
lnA_tryUsage( lnA_Parser* par, lnA_Usage* usg, char** argv ) {
    par->uNow = usg;
    par->uIdx = 0;
    par->argv = argv;
    par->aIdx = 0;
    
    return parseUsage( par );
}



#define uPeek( p ) ((p)->uNow->usage[(p)->uIdx])
#define uNext( p ) ((p)->uNow->usage[(p)->uIdx++])
#define uAdv( p )  ((p)->uIdx++)
#define aPeek( p ) ((p)->argv[(p)->aIdx])
#define aAdv( p )  ((p)->aIdx++)

static char*
parseThing( lnA_Parser* par );

static void
queueCallback( 
    lnA_Parser* par,
    void        (*cb)( char* str, void* udata ),
    char*       str
);

static void
queueCallbacks( lnA_Parser* par, lnA_Queue* src );

static void
invokeCallbacks( lnA_Parser* par );

static void
freeCallbacks( lnA_Parser* par );

static lnA_Param*
findParam( lnA_Parser* par, char* name, unsigned len );

static lnA_Option*
findOptionLong( lnA_Parser* par, char* name, unsigned len );

static lnA_Option*
findOptionShort( lnA_Parser* par, char name );

static char*
error( lnA_Parser* par, char* fmt, ... );

static char*
validate( lnA_Parser* par );

static char*
parseUsage( lnA_Parser* par ) {
    char* err = validate( par );
    if( err )
        return err;
    
    lnA_Queue q = { 0 };
    par->qNow = &q;
    
    while( uPeek( par ) != '\0' ) {
        err = parseThing( par );
        if( err )
            return err;
    }
    
    if( aPeek( par ) ) {
        return error( par, "Extra or unmatched word '%s'", aPeek( par ) );
    }
    invokeCallbacks( par );
    freeCallbacks( par );
    return NULL;
}

static bool
isOptChr( char* c ) {
    if( c[0] == '.' && c[1] == '.' && c[2] == '.' )
        return false;
    else
        return isgraph( *c ) &&
                *c != '[' && *c != ']' &&
                *c != '{' && *c != '}';
}

static char*
parseLong( lnA_Parser* par ) {
    
    // Find option name end in usage string
    int   uBrk = 0;
    char* uStr = &uPeek( par );
    while( isOptChr( &uStr[uBrk] ) && uStr[uBrk] != '=' )
        uBrk++;
    int uLen = uBrk;
    while( isOptChr( &uStr[uLen] ) )
        uLen++;
    
    par->uIdx += uLen;
    
    // Find matching option
    lnA_Option* opt = findOptionLong( par, uStr, uBrk );
    if( !opt )
        return error( par, "Missing option info" );
    
    // Make sure the argument is provided and is an option
    char* arg = aPeek( par );
    if( !arg || arg[0] != '-' || arg[1] != '-'  )
        return error( par, "Missing --%s option", opt->lForm );
    
    // Find option name end in argument string
    int   aBrk = 0;
    char* aStr = &arg[2];
    while( isgraph( aStr[aBrk] ) && aStr[aBrk] != '=' )
        aBrk++;
    
    // Make sure the two names match
    if( aBrk != uBrk || strncmp( aStr, uStr, aBrk ) )
        return error( par, "Missing --%s option", opt->lForm );
    
    // If usage string doesn't show option parameter
    // then the argument string shouldn't provide one
    if( uStr[uBrk] != '=' && aStr[aBrk] == '=' )
        return error( par, "Unexpected argument for --%s option", opt->lForm );
    
    // If an option argument is expected then the
    // argument string should provide one
    if( uStr[uBrk] == '=' && aStr[aBrk] != '=' )
        return error( par, "Missing argument for --%s option", opt->lForm );
    
    // Queue callbacks, will be called only if
    // unit completes without errors
    if( opt->callback )
        queueCallback( par, opt->callback, opt->lForm );
    lnA_Param* prm = findParam( par, &uStr[uBrk+1], uLen - uBrk - 1 );
    if( prm && prm->callback )
        queueCallback( par, prm->callback, &aStr[aBrk+1] );
    
    aAdv( par );
    return NULL;
}

static bool
contains( char* str, unsigned len, char chr ) {
    for( unsigned i = 0 ; i < len ; i++ ) {
        if( str[i] == chr )
            return true;
    }
    return false;
}

static char*
parseShort( lnA_Parser* par ) {
    
    // Allowed flags
    char* fStr = &uPeek( par );
    int   fLen = 0;
    while( isOptChr( &fStr[fLen] ) )
        fLen++;
    par->uIdx += fLen;
    
    // Make sure the argument is provided and is an option
    char* arg = aPeek( par );
    if( !arg || arg[0] != '-' || arg[1] == '-' || !isgraph( arg[1] ) )
        return error( par, "Missing -%.*s flag(s)", fLen, fStr );
    
    char* aChr = &arg[1];
    while( *aChr ) {
        if( !contains( fStr, fLen, *aChr ) )
            return error( par, "Invalid flag '%c' for -%.*s flag(s)", *aChr, fLen, fStr );
        aChr++;
    }
    aChr = &arg[1];
    while( *aChr ) {
        // Queue option callback if provided
        lnA_Option* opt = findOptionShort( par, *aChr );
        if( !opt )
            return error( par, "Missing option info" );
        if( opt->callback )
            queueCallback( par, opt->callback, opt->sForm );
        aChr++;
    }
    
    aAdv( par );
    return NULL;
}

static char*
parseParam( lnA_Parser* par ) {
    char*    uStr = &uPeek( par );
    unsigned uLen = 0;
    while( isOptChr( &uStr[uLen] ) )
        uLen++;
    par->uIdx += uLen;
    
    char* arg = aPeek( par );
    if( !arg || arg[0] == '-' )
        return error( par, "Missing %.*s parameter", uLen, uStr );
    
    lnA_Param* prm = findParam( par, uStr, uLen );
    if( prm && prm->callback )
        queueCallback( par, prm->callback, arg );
    
    aAdv( par );
    return NULL;
}

static void
exitGroup( lnA_Parser* par, char open, char close ) {
    while( uPeek( par ) != close ) {
        char c = uNext( par );
        if( c == open )
            exitGroup( par, open, close );
    }
    
    // Skip terminating bracket
    uAdv( par );
}

static char*
parseGroup( lnA_Parser* par, char open, char close ) {
    
    char* uStart = &uPeek( par ) - 1;

    // Replace the current callback queue with
    // one local to the current group, this allows
    // us to discard local callbacks for a failed
    // match while maintaining those for previous
    // successful matches
    lnA_Queue* oldQ = par->qNow;
    lnA_Queue  newQ = { 0 };
    par->qNow = &newQ;
    
    char* err;
again:
    // Parse all words/things in the current alternative
    err = NULL;
    while( !err && uPeek( par ) != '|' && uPeek( par ) != close ) {
        if( uPeek( par ) == '\0' )
            return error( par, "Unterminated group" );
        err = parseThing( par );
    }
    
    // If the match was successful then merge the
    // parent and local queues
    if( !err ) {
        // Skip everything until (and including) the closing bracket
        exitGroup( par, open, close );
        
        // Restore the old queue and merge it with the new one
        par->qNow = oldQ;
        queueCallbacks( par, &newQ );
        
        return NULL;
    }
    
    // If match fails then clear the queued callbacks
    freeCallbacks( par );
    
    // Skip until '[' or '|'
    while( uPeek( par ) != ']' && uPeek( par ) != '|' )
        uAdv( par );
    
    // If match fails but a '|' indicates a following
    // alternative then try again with the new form
    if( uPeek( par ) == '|' ) {
        uAdv( par );
        goto again;
    }
    
    // Restore old callback queue
    par->qNow = oldQ;
    
    // Skip the closing bracket
    uAdv( par );
    
    // Return error
    {
        char* uEnd = &uPeek( par ) + 1;
        int   uLen = uEnd - uStart;
        return error( par, "Missing group %.*s", uLen, uStart );
    }
}

static char*
parseThing( lnA_Parser* par ) {
    // Skip whitespace
    while( isspace( uPeek( par ) ) )
        uAdv( par );
    
    // Parse unit
    unsigned uStart = par->uIdx;
    bool parsedOne = false;
    char* err = NULL;
again:
    switch( uPeek( par ) ) {
        case '-':
            uAdv( par );
            if( uPeek( par ) == '-' ) {
                uAdv( par );
                err = parseLong( par );
            }
            else {
                err = parseShort( par );
            }
        break;
        case '[':
            uAdv( par );
            err = parseGroup( par, '[', ']' );
            parsedOne = true;
        break;
        case '{':
            uAdv( par );
            err = parseGroup( par, '{', '}' );
        break;
        default:
            err = parseParam( par );
        break;
    }
    char* end = &uPeek( par );
    if( end[0] == '.' && end[1] == '.' && end[2] == '.' ) {
        if( !err ) {
            par->uIdx = uStart;
            parsedOne = true;
            goto again;
        }
        
        par->uIdx += 3;
    }

    // Skip whitespace
    while( isspace( uPeek( par ) ) )
        uAdv( par );
    
    if( parsedOne )
        return NULL;
    else
        return err;
}


static void
queueCallback( 
    lnA_Parser* par,
    void        (*cb)( char* str, void* udata ),
    char*       str
) {
    lnA_Queued* c = malloc(sizeof(lnA_Queued));
    c->callback = cb;
    c->str = str;
    c->next = NULL;
    if( par->qNow->last ) {
        par->qNow->last->next = c;
        par->qNow->last = c;
    }
    else {
        par->qNow->first = c;
        par->qNow->last  = c;
    }
}

static void
queueCallbacks( lnA_Parser* par, lnA_Queue* src ) {
    if( par->qNow->last ) {
        par->qNow->last->next = src->first;
        par->qNow->last = src->last;
    }
    else {
        par->qNow->first = src->first;
        par->qNow->last  = src->last;
    }
}

static void
invokeCallbacks( lnA_Parser* par ) {
    lnA_Queued* qIt = par->qNow->first;
    while( qIt ) {
        qIt->callback( qIt->str, par->udata );
        qIt = qIt->next;
    }
}

static void
freeCallbacks( lnA_Parser* par ) {
    lnA_Queued* qIt = par->qNow->first;
    while( qIt ) {
        lnA_Queued* tmp = qIt;
        qIt = qIt->next;
        free( tmp );
    }
    *par->qNow = (lnA_Queue){ 0 };
}

static lnA_Param*
findParam( lnA_Parser* par, char* name, unsigned len ) {
    lnA_Param* pIt = par->pList;
    while( pIt ) {
        unsigned pLen = strlen( pIt->name );
        if( len == pLen && !strncmp( name, pIt->name, len ) )
            return pIt;
        pIt = pIt->next;
    }
    return NULL;
}

static lnA_Option*
findOptionLong( lnA_Parser* par, char* name, unsigned len ) {
    lnA_Option* oIt = par->oList;
    while( oIt ) {
        lnA_Option* opt = oIt;
        oIt = oIt->next;
        
        if( !opt->lForm )
            continue;
        
        unsigned oLen = strlen( opt->lForm );
        if( len == oLen && !strncmp( name, opt->lForm, len ) )
            return opt;
    }
    return NULL;
}

static lnA_Option*
findOptionShort( lnA_Parser* par, char name ) {
    lnA_Option* oIt = par->oList;
    while( oIt ) {
        lnA_Option* opt = oIt;
        oIt = oIt->next;
        
        if( !opt->sForm )
            continue;
        
        if( contains( opt->sForm, strlen(opt->sForm), name ) )
            return opt;
        opt = opt->next;
    }
    return NULL;
}

static char*
error( lnA_Parser* par, char* fmt, ... ) {
    va_list args;
    va_start( args, fmt );
    unsigned len = vsnprintf( NULL, 0, fmt, args ) + 1;
    va_end( args );
    
    par->eText = realloc( par->eText, len );
    va_start( args, fmt );
    vsnprintf( par->eText, len, fmt, args );
    va_end( args );
    
    return par->eText;
}

static char*
validateRequired( lnA_Parser* par );

static char*
validateOptional( lnA_Parser* par ) {
    
    // Skip whitespace
    while( isspace( uPeek( par ) ) )
        uAdv( par );
    
    while( uPeek( par ) != ']' ) {
        char c = uNext( par );
        if( c == '}' || c == '\0' )
            return error( par, "Unterminated optional group" );
        if( c == '[' ) {
            char* err = validateOptional( par );
            if( err )
                return err;
        }
        if( c == '{' ) {
            char* err = validateRequired( par );
            if( err )
                return err;
        }
    }
    
    // Skip terminating bracket
    uAdv( par );
    return NULL;
}

static char*
validateRequired( lnA_Parser* par ) {
    
    // Skip whitespace
    while( isspace( uPeek( par ) ) )
        uAdv( par );
    
    while( uPeek( par ) != '}' ) {
        char c = uNext( par );
        if( c == ']' || c == '\0' )
            return error( par, "Unterminated optional group" );
        if( c == '[' ) {
            char* err = validateOptional( par );
            if( err )
                return err;
        }
        if( c == '{' ) {
            char* err = validateRequired( par );
            if( err )
                return err;
        }
    }
    
    // Skip terminating bracket
    uAdv( par );
    return NULL;
}

static char*
validate( lnA_Parser* par  ) {
    unsigned uIdx = par->uIdx;
    // Skip whitespace
    while( isspace( uPeek( par ) ) )
        uAdv( par );
    
    while( uPeek( par ) != '\0' ) {
        char c = uNext( par );
        if( c == ']' || c == '}' )
            return error( par, "Stray bracket" );
        if( c == '[' ) {
            char* err = validateOptional( par );
            if( err )
                return err;
        }
        if( c == '{' ) {
            char* err = validateRequired( par );
            if( err )
                return err;
        }
    }
    par->uIdx = uIdx;
    return NULL;
}
