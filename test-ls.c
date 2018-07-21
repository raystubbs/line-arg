#include "line-arg.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Text copied from GNU ls help message

lnA_Parser* par = NULL;

bool  fAll       = false;
bool  fAlmostAll = false;
bool  fEscape    = false;
bool  fLine      = false;
int   iWidth     = 0;
char* sFile      = NULL;


void
flagCb( char* flag ) {
    switch( *flag ) {
        case 'a':
            fAll = true;
        break;
        case 'A':
            fAlmostAll = true;
        break;
        case 'b':
            fEscape = true;
        break;
        case 'l':
            fLine = true;
        break;
        default:
            assert( false );
        break;
    }
}

void
helpCb( char* _ ) {
    lnA_printUsage( par );
    lnA_freeParser( par );
    exit( 0 );
}

void
widthCb( char* width ) {
    iWidth = atoi( width );
}

void
fileCb( char* file ) {
    sFile = file;
}

int
main( int argc, char** argv ) {
    par = lnA_makeParser( "ls" );
    
    // The usage strings determine all valid uses of
    // the program, any deviaitons will cause an error;
    // multiple usage strings can be added to any parser
    lnA_Usage* usg =
        lnA_addUsage(
            par,
            "[-Aablh | --width=WIDTH | -w WIDTH | --help]... [FILE]"
        );
    
    // The header text is added to the top of the usage
    // message, right after the usage strings
    lnA_setHeader(
        par,
        "List information about the FILEs (the current directory "
        "by default).\n"
        "Sort entries alphabetically if none of -cftuvSUX nor --sort "
        "is specified."
    );
    
    // Each option specified in the usage string must
    // be registered here, short forms or long forms
    // can be omitted asNULL but not both.  Short forms
    // are always only one character long, if multiple
    // characters are in the sf string then they're
    // interpretted as alternatives with the same meaning
    lnA_addOption(
        par, "a", "all",
        "do not ignore entries starting with .",
        &flagCb
    );
    lnA_addOption(
        par, "A", "almost-all",
        "do not list implied . and ..",
        &flagCb
    );
    lnA_addOption(
        par, "b", "escape",
        "print C-style escapes for nongraphic characters",
        &flagCb
    );
    lnA_addOption(
        par, "l", NULL,
        "list one file per line.  Avoid '\\n' with -q or -b",
        &flagCb
    );
    lnA_addOption(
        par, "w", "width",
        "set output width to COLS.  0 means no limit",
        NULL
    );
    lnA_addOption(
        par, "h", "help",
        "display this help and exit",
        &helpCb
    );
    
    
    // Registering parameters isn't required, but is
    // necessary to for a program to be notified of
    // received arguments; the parameter name matches
    // the name specified in the usage string, either
    // as a solo word or as the right-hand side of a
    // '=' in a long option.  Callbacks will be called
    // in the order in which the parameters were read
    lnA_addParam( par, "WIDTH", &widthCb );
    lnA_addParam( par, "FILE", &fileCb );
    
    // The footer is printed after all options
    lnA_setFooter(
        par,
        "The SIZE argument is an integer and optional unit (example: 10K is 10*1024).\n"
        "Units are K,M,G,T,P,E,Z,Y (powers of 1024) or KB,MB,... (powers of 1000).\n"
        "\n"
        "Using color to distinguish file types is disabled both by default and\n"
        "with --color=never.  With --color=auto, ls emits color codes only when\n"
        "standard output is connected to a terminal.  The LS_COLORS environment\n"
        "variable can change the settings.  Use the dircolors command to set it.\n"
        "\n"
        "Exit status:\n"
        "0  if OK,\n"
        "1  if minor problems (e.g., cannot access subdirectory),\n"
        "2  if serious trouble (e.g., cannot access command-line argument).\n"
    );
    
    // We need to try each usage string independently,
    // line-arg leaves it to the user to determine
    // which usage error to print since each usage
    // alternative will have a different one.  In our
    // simple case there's only one usage alternative,
    // so there's no real issue.  This functions returns
    // NULL on success or an error message on error.
    char* err = lnA_tryUsage( par, usg, &argv[1] );
    if( err ) {
        fprintf( stderr, "Error: %s\n", err );
        lnA_freeParser( par );
        exit( 1 );
    }
    
    // This isn't very robust... but it isn't the focus
    // of the example
    char cmd[1024];
    snprintf(
        cmd, 1024,
        "ls %s %s %s %s --width=%i %s",
        fAll ? "-a" : "",
        fAlmostAll ? "-A" : "",
        fEscape ? "-b" : "",
        fLine ? "-l" : "",
        iWidth,
        sFile ? sFile : ""
    );
    system( cmd );
    lnA_freeParser( par );
    return 0;
}
