#include "line-arg.h"
#include <stddef.h>

typedef enum Type {
    Flag,
    Enum,
    String,
    Number,
    Path,
    Metric,
    Custom
} Type;

typedef struct PathEntry PathEntry;
struct PathEntry {
    char const* ext;
    char const* desc;
    PathEntry*  next;
};

typedef struct PathClass PathClass;
struct PathClass {
    PathEntry* entries;
    PathClass* next;
};

typedef struct UnitEntry UnitEntry;
struct UnitEntry {
    char const* sym;
    char const* desc;
    UnitEntry*  next;
};

typedef struct UnitClass UnitClass;
struct UnitClass {
    UnitEntry* entries;
    UnitClass* next;
};


typedef struct EnumEntry EnumEntry;
struct EnumEntry {
    char const* name;
    EnumEntry*  next;
};

typedef struct EnumClass EnumClass;
struct EnumClass {
    EnumEntry* entries;
    EnumClass* next;
};


typedef struct Option Option;
struct Option {
    char const*  lForm;
    char const*  sForm;
    Option*      pAfter;
    unsigned     pLen;
    Type         type;
    void*        ptr;
    lnA_OptionCb cb;
    bool         isList;
    bool         isDone;
    bool         isRequired;
    char const*  description;
    Option*      onlyIf;
    Option*      reqIf;
    Option*      notIf;
    PathClass*   pClass;
    UnitClass*   uClass;
    EnumClass*   eClass;
    Option*      next;
};

typedef struct Parser  Parser;
typedef struct Command Command;
struct Command {
    char const* cmd;
    Parser*     par;
    Command*    next;
};

struct Parser {
    bool         isDone;
    lnA_ErrInfo* eInfo;
    Option*      pStart;
    Option*      oList;
    Command*     cList;
} Parser;

struct Stream {
    int          next;
    unsigned     iloc;
    unsigned     jloc;
    const char** argv;
};


lnA_Parser*
lnA_makeParser( lnA_ErrInfo* eInfo ) {
    Parser* par = malloc( sizeof(Parser) );
    par->isDone = false;
    par->eInfo  = eInfo;
    par->pStart = NULL;
    par->oList  = NULL;
    par->cList  = NULL;
    return (lnA_Parser*)par;
}

static void
freeOption( Option* opt );

static void
freeCommad( Command* cmd );

void
lnA_freeParser( lnA_Parser* ipar ) {
    Parser* par = (Parser*)ipar;
    
    Option* oIt = par->oInfo;
    while( oIt ) {
        Option* tmp = oIt;
        oIt = oIt->next;
        freeOption( tmp );
    }
    
    Command* cIt = par->cInfo;
    while( cIt ) {
        Command* tmp = par->cInfo;
        cIt = cIt->next;
        freeCommand( tmp->par );
    }
    
    free( par );
}

static void
parse( Parser* par, Stream* src );

void
lnA_parseOptions( lnA_Parser* ipar, char const** argv ) {
    Parser* par = (Parser*)par;
    Stream* src = {.next = argv[0][0],
                   .iloc = 0,
                   .jloc = 1,
                   .argv = argv
                  };
    parse( par, src );
}


// Other API functions

static void
advance( Stream* src );

static char const*
parseWord( Parser* par, Stream* src ) {
    
    // Save the word's start location
    char const* loc = &src->argv[src->iloc][src->jloc];
    
    // Figure out how long the word is
    size_t len = 0;
    while( src->next != '\0' ) {
        advance( src );
        len++;
    }
    
    // Copy the string to a new allocation
    char* cpy = malloc( len + 1 );
    strncpy( cpy, loc, len );
    
    return cpy;
}

static char const*
maybeStr( Stream* src, char const* str ) {
    // Save start location
    size_t iloc = src->iloc;
    size_t jloc = src->jloc;
    
    // Compare stream with string, stream token
    // must be null terminated to qualify
    while( src->next == *str && src->next && *str ) {
        advance( src );
        str++;
    }
    
    if( !src->next && !*str ) {
        advance( src );
        return src->argv[iloc][jloc];
    }
    
    src->iloc = iloc;
    src->jloc = jloc;
    return NULL;
}

static void
error( Parser* par, lnA_ErrType type, char const* fmt, ... );

static void
parseFlag( Parser* par, Stream* src, Option* opt ) {
    lnA_Flag* val = (lnA_Flag*)opt->ptr;
    
    // This will be called after the flag name
    // has already been parsed, so if there's
    // no value then we just set it to true
    if( src->next == '\0' ) {
        *val = true;
        return;
    }
    
    // We accept the following as valid flag values:
    //  - true, TRUE, True, T, t, yes, YES, Yes, Y, y
    //  - false, FALSE, False, F, f, no, NO, No, N, n
    if(
        maybeStr( "true" ) ||
        maybeStr( "TRUE" ) ||
        maybeStr( "True" ) ||
        maybeStr( "T" )    ||
        maybeStr( "t" )    ||
        maybeStr( "yes" )  ||
        maybeStr( "YES" )  ||
        maybeStr( "Yes" )  ||
        maybeStr( "Y" )    ||
        maybeStr( "y" )
    ) { 
        *val = true;
        return;
    }
    
    if(
        maybeStr( "false" ) ||
        maybeStr( "FALSE" ) ||
        maybeStr( "False" ) ||
        maybeStr( "F" )     ||
        maybeStr( "f" )     ||
        maybeStr( "no" )    ||
        maybeStr( "NO" )    ||
        maybeStr( "No" )    ||
        maybeStr( "N" )     ||
        maybeStr( "n" )
    ) {
        *val = false;
        return;
    }
    
    error(
        par, lnA_ERR_OPTION,
        "Invalid flag value '%s' for '%s', "
        "try '--help .flags'",
        src->argv[src->iloc], opt->lForm
    );
}

static void
parseEnum( Parser* par, Stream* src, Option* opt ) {
    lnA_Enum* val = (lnA_Enum*)opt->ptr;
    
    char const* loc = NULL;
    EnumEntry*  eIt = opt->eClass->entries;
    while( eIt && !loc ) {
        loc = maybeStr( src, eIt->name ) );
        eIt = eIt->next;
    }
    
    if( loc ) {
        size_t len = strlen( loc );
        char*  cpy = malloc( len + 1 );
        strcpy( cpy, loc );
        *val = cpy;
        return;
    }
    
    error(
        par, lnA_ERR_OPTION,
        "Invalid enumeration value '%s' for '%s', "
        "try '--help .enums'",
        &src->argv[src->iloc][src->jloc], opt->lForm
    );
}

static void
parseString( Parser* par, Stream* src, Option* opt ) {
    lnA_String* val = (lnA_String*)opt->ptr;
    *val = parseWord( par, src );
}

static void
parseNumber( Parser* par, Stream* src, Option* opt ) {
    lnA_Number* val = (lnA_Number*)opt->ptr;
    char const* loc = &src->argv[src->iloc][src->jloc];
    char*  end;
    double num = strtod( loc, &end );
    if( !*end ) {
        *val = num;
        return;
    }
    
    error(
        par, lnA_ERR_OPTION,
        "Invalid number value '%s' for '%s', "
        "try '--help .numbers'",
        loc, opt->lForm
    );
}

static void
parsePath( Parser* par, Stream* src, Option* opt ) {

}


static void
parseLongOption( Parser* par, Stream* src ) {

}

static void
parseShortOption( Parser* par, Stream* src ) {

}

static void
parseParamOption( Parser* par, Stream* src ) {

}
