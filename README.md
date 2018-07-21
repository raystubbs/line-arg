# line-arg (or lnA)
A simple parser for command line arguments that uses one or
more usage strings to determine the validity of a user's
argument set.  The library is capable of using a simple
usage string with standard formatting as a reference to
parse user arguments.

## Syntax
lnA is simple and small, so it doesn't support any complex or
less known usage specification conventions.  Basically what
it understands are:

### Parameters
These are just names without any associated options, the
word parsed at the same location as the parameter name
is passed to the parameter callback bound to the name.

    "MY-PARAM"

The name can have basically any graphic ASCII characters
except '[', ']', '{', and '}'.  They also can't begin with
a hyphen '-' since that denotes options.  They also can't
contain three consecutive periods '...' since that denotes
a sequence.

### Short Options
Short options begin with a single hyphen '-' and are
followed by a set of letters terminating in whitespace.
These letters represent alternatives; they are not names,
each letter is the short form for an option flag; and the
user arguments can have one or more of the specified letters
at the matching location.

    "-AaBb"

For each valid letter in the matching user argument lnA will
call the matching option callback; the callback will receive
the short option string of the matched option, not the user
argument.

### Long Options
Long options begin with a double hyphen '--' and are
followed by a set of letters terminating in whitespace
or by an equal sign '=' and a parameter name.  When
matching the first case only the option callback will
be called, but in the second case the option and parameter
callbacks will be called.  The option callback will be passed
the matching option's long form string.  The parameter callback
will be passed the matching user argument.

    "--help"
    "--width=WIDTH"

### Optional Groups
Optional groups represent a set of options/parameters that
may or may not be present in the user's argument set; if
they're found then the appropriate callbacks will be invoked,
otherwise they won't.  Optional groups are enclosed in square
brackets [...] and can be nested.  Alternative forms of an
optional group are separated by bars '|'.  Alternatives are
attempted in the order in which they're given, so if an
argument set can match multiple alternatives then the left-most
one will be used.

    "[-Aablh | --width=WIDTH | -w WIDTH | --help]"
    "[FILES...]"

### Required Groups
Required groups are similar to optional groups in form, but
are required to be present in at least one valid form.  When
matching sequences at least one match is required.  Required
groups are enclosed in curly brackets {...} and alternatives
are separated by bars '|'.

    "{-a | -b | -c | -d}"

### Sequences
Sequences are denoted by '...' after one of the previously
described forms.  This indicates that the form should be
repeated as many times as can be matched, but must be
matched at least once, except for sequences of optional
groups.

    "FILES..."
    "{-i | -o}..."
    "[-f]..."
    "-AaBb..."
    "--something=otherthing..."

### Whitespace
Whitespace is ignore except where it serves as a delimiter.

## Building
Build a shared library with:

    make shared

A static library with:

    make static

These will only work in GNU environments, but lnA is written
in portable C99, so simply compiling the 'line-arg.c' file with
any modern compiler should do the trick.

## Usage
lnA is intended to be delightfully simple to use, no need to
remember a bunch of struct formatns and whatnot; valid usage
forms are represented by the same strings that are used to
generate the usage text, which follow common unix conventions.

We only need one header file:

    #include <line-arg.h>

But for printing and whatnot we use some others:

    #include <stdlib.h>
    #include <stdio.h>

To create a parser we just say:

    lnA_Parser* par = lnA_makeParser( "programName" );

Next we add a usage string:

    lnA_Usage* usg = lnA_addUsage( par, "{ params... | [-h | --help] }" );

This represents a parser for that specific usage string,
when multiple usage options are possible it becomes difficult
to decide which error message to display on failure, so lnA
defers this responsibility to the user by making them try
each usage manually and report or ignore the reported error.

Next we need to tell the parser what each option means, 
and what to do when we encounter them:

    lnA_addOption( par, "h", "help", "Displays usage info", &helpCb );

And elsewhere, assuming 'par' is a global parser instance:

    void
    helpCb( char* opt ) {
        lnA_printUsage( par );
        exit( 1 );
    }

The first argument (after the parser) is the short form of the
option, this can be null to indicate that there is no short
form.  If multiple letters are given in the short form these
are taken to be alternatives with the same meaning, a short
option can only consist of one letter.  The next string is
the long form, this is taken literally an matched letter by
letter with the user passed options.  The next string is the
description, this is used for printing the help/usage message.
The last argument is a callback to be called when the option is
encountered.

Next we can (optional) add parameter handles; these are callbacks
that are invoked when the specified parameter is matched.  They'll
be called in the order in which the parameters are read, from left
to right.

    lnA_addParam( par, "params", &paramCb );

And elsewhere:

    void
    paramCb( char* arg ) {
        printf( "Got argument: %s\n", arg );
    }

Now we just add optional details to our usage message.  The
header and footer are printed before and after (respectively)
the option list.

    lnA_setHeader( par, "Header text" );
    lnA_setFooter( par, "Footer text" );

And then we try parsing, for each lnA_Usage alternative we
need to make a call to lnA_tryUsage(), which will return
NULL on success or an error message on failure.  It's left
to the user to decide which error messages to print, if any.
Note that we only pass the arguments from argv[] into the
call by starting at index 1; lnA tries to parse everything
it gets, it won't skip the command string.

    char* err = lnA_tryUsage( par, usg, &argv[1] );
    if( err ) {
        fprintf( stderr, "Error: %s\n", err );
        lnA_freeParser( par );
        exit( 1 );
    }

Note that the error message will only be available until
the next call to lnA_tryUsage().

And finally we cleanup once we're done with the parser:

    lnA_freeParser( par );

Note that lnA doesn't copy strings, so any strings passed
into its functions are expected to be static, or at least
be available throughout the lifetime of the parser. Now
what we have is:

    #include <stdlib.h>
    #include <stdio.h>
    #include <line-arg.h>
    
    lnA_Parser* par = NULL;
    lnA_Usage*  usg = NULL;
    
    void
    helpCb( char* opt ) {
        lnA_printUsage( par );
        exit( 1 );
    }
    
    void
    paramCb( char* arg ) {
        printf( "Got argument: %s\n", arg );
    }
    
    
    int
    main( int argc, char** argv ) {
        par = lnA_makeParser( "programName" );
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

And we can build with something like:

    cd line-arg
    make
    cd ..
    gcc -Lline-arg -llnA -Iline-arg my-program.c

Or perhaps more simply as:

    gcc -Iline-arg line-arg/line-arg.c my-program.c


**Note** the order of our usage string, lnA isn't very smart;
it tries alternatives in order from left to right.  So if we
reverse the order of our alternatives and say "{ [-h | --help] | params...] }"
it'll first attempt to match the optional first alternative, failing
that it won't try any others or throw an error since the form is
optional.

**Note** that lnA is very young, I just slapped together the few
hundred lines today (July 20, 2018); so there will likely be plenty
of bugs.  If you find any, or would like a feature implemented, then
please let me know.  I'm already planning on adding support for callback
userdata, so don't bother requesting that.
