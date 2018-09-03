#include <stdlib.h>
#include <stdio.h>
#include "line-arg.h"

lnA_Parser* par = NULL;
lnA_Usage*  usg = NULL;

void
helpCb( char* opt, void* udata ) {
    lnA_printUsage( par );
    exit( 1 );
}

void
paramCb( char* arg, void* udata ) {
    printf( "Got argument: %s\n", arg );
}

int
main( int argc, char** argv ) {
    par = lnA_makeParser( "programName", NULL );
    usg = lnA_addUsage( par, "{ params... | [-h | --help] }" );
    
    lnA_addOption( par, "h", "help", "Displays usage info", &helpCb );
    lnA_addParam( par, "params", &paramCb );
    lnA_setHeader( par, "Header text" );
    lnA_setFooter( par, "Footer text" );
    
    char* err = lnA_tryUsage( par, usg, &argv[1] );
    if( err ) {
        fprintf( stderr, "Error: %s\n", err );
        lnA_freeParser( par );
        exit( 1 );
    }
    
    lnA_freeParser( par );
    return 0;
}
