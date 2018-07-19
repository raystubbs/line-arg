#ifndef lnA_line_arg_h
#define lnA_line_arg_h
#include <stdbool.h>

typedef struct lnA_Parser    lnA_Parser;
typedef struct lnA_Option    lnA_Option;
typedef struct lnA_PathClass lnA_PathClass;
typedef struct lnA_UnitClass lnA_UnitClass;
typedef struct lnA_EnumClass lnA_EnumClass;

char const*
(*lnA_CustomCb)( lnA_Config* config, char const* opt, char const* arg );

typedef bool   lnA_Flag;
typedef char*  lnA_Enum;
typedef char*  lnA_String;
typedef double lnA_Number;
typedef struct lnA_Path {
    char* type;
    char* path;
} lnA_Path;

typedef struct lnA_Metric {
    char*  unit;
    double scale;
} lnA_Metric;

typedef enum lnA_ErrType {
    lnA_ERR_SYSTEM,
    lnA_ERR_USER,
    lnA_ERR_OPTION
} lnA_ErrType;

typedef struct lnA_ErrInfo {
    jmp_buf     jmp;
    lnA_ErrType type;
    char const* msg;
} lnA_ErrInfo;

lnA_Parser*
lnA_makeParser( lnA_ErrInfo* eInfo );

void
lnA_freeParser( lnA_Parser* par );

void
lnA_parseOptions( lnA_Parser* par, char const** argv );

lnA_Parser*
lnA_subCommand( lnA_Parser* par, char const* cmd );

lnA_PathClass*
lnA_addPathClass( lnA_Parser* par );

void
lnA_addPathType( lnA_PathClass* pc, const char* ext, const char* desc );

lnA_UnitClass*
lnA_addUnitClass( lnA_Parser* par );

void
lnA_addUnitType( lnA_UnitClass* uc, const char* sym, const char* desc );

lnA_EnumClass*
lnA_addEnumClass( lnA_Parser* par );

void
lnA_addEnumName( lnA_EnumClass* ec, const char* name )

lnA_Option*
lnA_addFlagA( lnA_Parser* par, char const* name, lnA_Flag* f );

lnA_Option*
lnA_addEnumA( lnA_Parser* par, char const* name, lnA_Enum* e );

lnA_Option*
lnA_addStringA( lnA_Parser* par, char const* name, lnA_String* s );

lnA_Option*
lnA_addNumberA( lnA_Parser* par, char const* name, lnA_Number* n );

lnA_Option*
lnA_addPathA( lnA_Parser* par, char const* name, lnA_Path* p );

lnA_Option*
lnA_addMetricA( lnA_Parser* par, char const* name, lnA_Metric* p );

lnA_Option*
lnA_addCustomA( lnA_Parser* par, char const* name, lnA_CustomCb c );

lnA_Option*
lnA_addFlagL( lnA_Parser* par, char const* name, lnA_Flag** f );

lnA_Option*
lnA_addEnumL( lnA_Parser* par, char const* name, lnA_Enum** e );

lnA_Option*
lnA_addStringL( lnA_Parser* par, char const* name, lnA_String** s );

lnA_Option*
lnA_addNumberL( lnA_Parser* par, char const* name, lnA_Number** n );

lnA_Option*
lnA_addPathL( lnA_Parser* par, char const* name, lnA_Path** p );

lnA_Option*
lnA_addMetricL( lnA_Parser* par, char const* name, lnA_Metric** p );

lnA_Option*
lnA_addCustomL( lnA_Parser* par, char const* name, lnA_CustomCb c );

void
lnA_setDescription( lnA_Option* opt, char const* desc );

void
lnA_setShortForm( lnA_Option* opt, char const* sf );

void
lnA_setParamForm( lnA_Option* opt, lnA_Option* after );

void
lnA_setRequired( lnA_Option* opt );

void
lnA_setOnlyIf( lnA_Option* opt, lnA_Option* depends );

void
lnA_setReqIf( lnA_Option* opt, lnA_Option* depends );

void
lnA_setNotIf( lnA_Option* opt, lnA_Option* depends );

void
lnA_setPathClass( lnA_Option* opt, lnA_PathClass* pc );

void
lnA_setUnitClass( lnA_Option* opt, lnA_UnitClass* uc );

void
lnA_setEnumClass( lnA_Option* opt, lnA_EnumClass* ec );

#endif
