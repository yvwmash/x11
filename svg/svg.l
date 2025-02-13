%{
#include <stdio.h>

#include "yy.svg_docu.tab.h"

#define ATTRIB_TYP_END        -1
#define ATTRIB_TYP_ERROR       0
#define ATTRIB_TYP_PATH        1
#define ATTRIB_TYP_WIDTH       2
#define ATTRIB_TYP_HEIGHT      3
#define ATTRIB_TYP_VIEWBOX     4
#define ATTRIB_TYP_VERSION     5
#define ATTRIB_TYP_ID          6
#define ATTRIB_TYP_XMLNS       7
#define ATTRIB_TYP_XMLNS_SVG   8
#define ATTRIB_TYP_STYLE       9
#define ATTRIB_TYP_ENCODING    10
#define ATTRIB_TYP_STANDALONE  11

static int nline        = 1;
static int attrib_typ   = ATTRIB_TYP_ERROR;

#define PUT_STOKEN() yy_svg_docu_lval.sval.ps = yytext; yy_svg_docu_lval.sval.len = yyleng;

#define SWITCH_ATTRIB_TYP(t) { switch(t) {\
							   case ATTRIB_TYP_END         : BEGIN(ATTRIBUTES);      break;\
							   case ATTRIB_TYP_PATH        : BEGIN(PATH_ATTRIBUTE);  break;\
							   case ATTRIB_TYP_WIDTH       : BEGIN(FLOAT_LIST);      break;\
							   case ATTRIB_TYP_HEIGHT      : BEGIN(FLOAT_LIST);      break;\
							   case ATTRIB_TYP_VIEWBOX     : BEGIN(FLOAT_LIST);      break;\
							   case ATTRIB_TYP_VERSION     : BEGIN(VERSION_STRING);  break;\
							   case ATTRIB_TYP_ID          : BEGIN(STRING);          break;\
							   case ATTRIB_TYP_XMLNS       : BEGIN(STRING);          break;\
							   case ATTRIB_TYP_XMLNS_SVG   : BEGIN(STRING);          break;\
							   case ATTRIB_TYP_STYLE       : BEGIN(STRING);          break;\
							   case ATTRIB_TYP_ENCODING    : BEGIN(STRING);          break;\
							   case ATTRIB_TYP_STANDALONE  : BEGIN(STRING);          break;\
							   default:                    BEGIN(ATTRIBUTE_ERROR); break;\
                               }\
                             }

%}

%option noyywrap

%x ATTRIBUTES ATTRIBUTE_Q ATTRIBUTE_ERROR FLOAT_LIST VERSION_STRING STRING PATH_ATTRIBUTE COMMENTS

%%
"<?xml"            { printf("OPEN_XML_DECL\n");    BEGIN(ATTRIBUTES);     return OPEN_XML_DECL; }
"<svg"             { printf("OPEN_SVG\n");         BEGIN(ATTRIBUTES);     return OPEN_SVG; }
"</svg>"           { printf("CLOSE_SVG\n");                               return CLOSE_SVG; }
"<defs"            { printf("OPEN_DEFS\n");        BEGIN(ATTRIBUTES);     return OPEN_DEFS; }
"<g"               { printf("OPEN_G\n");           BEGIN(ATTRIBUTES);     return OPEN_G; }
"</g>"             { printf("CLOSE_G\n");                                 return CLOSE_G; }
"<path"            { printf("OPEN_PATH\n");        BEGIN(ATTRIBUTES);     return OPEN_PATH; }
"<!--"             { printf("OPEN_COMMENTS\n");    BEGIN(COMMENTS);       return OPEN_COMMENTS; }

<COMMENTS>.*"-->"  { BEGIN(INITIAL); return CLOSE_COMMENTS; }

<ATTRIBUTES>"/>"                            { printf("SELF_CLOSING_TAG\n"); BEGIN(INITIAL);         return SELF_CLOSING_TAG; }
<ATTRIBUTES>">"                             { printf("TAG_E\n");            BEGIN(INITIAL);         return TAG_E; }
<ATTRIBUTES>"\?>"                           { printf("CLOSE_XML_DECL\n");   BEGIN(INITIAL);         return CLOSE_XML_DECL; }
<ATTRIBUTES>"d"                             { PUT_STOKEN() attrib_typ = ATTRIB_TYP_PATH;            return ATTRIBUTE_NM_D; }
<ATTRIBUTES>"encoding"                      { PUT_STOKEN() attrib_typ = ATTRIB_TYP_ENCODING;        return ATTRIBUTE_NM_ENCODING; }
<ATTRIBUTES>"standalone"                    { PUT_STOKEN() attrib_typ = ATTRIB_TYP_STANDALONE;      return ATTRIBUTE_NM_STANDALONE; }
<ATTRIBUTES>"width"                         { PUT_STOKEN() attrib_typ = ATTRIB_TYP_WIDTH;           return ATTRIBUTE_NM_WIDTH; }
<ATTRIBUTES>"height"                        { PUT_STOKEN() attrib_typ = ATTRIB_TYP_HEIGHT;          return ATTRIBUTE_NM_HEIGHT; }
<ATTRIBUTES>"viewBox"                       { PUT_STOKEN() attrib_typ = ATTRIB_TYP_VIEWBOX;         return ATTRIBUTE_NM_VIEWBOX; }
<ATTRIBUTES>"version"                       { PUT_STOKEN() attrib_typ = ATTRIB_TYP_VERSION;         return ATTRIBUTE_NM_VERSION; }
<ATTRIBUTES>"id"                            { PUT_STOKEN() attrib_typ = ATTRIB_TYP_ID;              return ATTRIBUTE_NM_ID; }
<ATTRIBUTES>"xmlns"                         { PUT_STOKEN() attrib_typ = ATTRIB_TYP_XMLNS;           return ATTRIBUTE_NM_XMLNS; }
<ATTRIBUTES>"xmlns:svg"                     { PUT_STOKEN() attrib_typ = ATTRIB_TYP_XMLNS_SVG;       return ATTRIBUTE_NM_XMLNS_SVG; }
<ATTRIBUTES>"style"                         { PUT_STOKEN() attrib_typ = ATTRIB_TYP_STYLE;           return ATTRIBUTE_NM_STYLE; }

<ATTRIBUTE_ERROR>.                          { return ATTRIBUTE_NM_ERROR; }

<ATTRIBUTES>"="                             { BEGIN(ATTRIBUTE_Q);                                   return '='; }
<ATTRIBUTE_Q>"\""|"\'"                      { PUT_STOKEN() SWITCH_ATTRIB_TYP(attrib_typ);           return ATTRIBUTE_QUOTE; }

<FLOAT_LIST>[+-]?([0-9]*[.])?[0-9]+         { PUT_STOKEN() return ATTRIBUTE_FLOAT_VALUE; }
<FLOAT_LIST>"\""|"\'"                       { PUT_STOKEN() BEGIN(ATTRIBUTES); return ATTRIBUTE_QUOTE; }

<VERSION_STRING>[0-9]+\.[0-9]+              { PUT_STOKEN() return ATTRIBUTE_VERSION_STRING_VALUE; }
<VERSION_STRING>"\""|"\'"                   { PUT_STOKEN() BEGIN(ATTRIBUTES); return ATTRIBUTE_QUOTE; }

<STRING>[^"']+                              { PUT_STOKEN() return ATTRIBUTE_STRING_VALUE; }
<STRING>"\""|"\'"                           { PUT_STOKEN() BEGIN(ATTRIBUTES); return ATTRIBUTE_QUOTE; }

<PATH_ATTRIBUTE>"M"                         { PUT_STOKEN() return 'M'; }
<PATH_ATTRIBUTE>"m"                         { PUT_STOKEN() return 'm'; }
<PATH_ATTRIBUTE>"l"                         { PUT_STOKEN() return 'l'; }
<PATH_ATTRIBUTE>"L"                         { PUT_STOKEN() return 'L'; }
<PATH_ATTRIBUTE>"h"                         { PUT_STOKEN() return 'h'; }
<PATH_ATTRIBUTE>"H"                         { PUT_STOKEN() return 'H'; }
<PATH_ATTRIBUTE>"v"                         { PUT_STOKEN() return 'v'; }
<PATH_ATTRIBUTE>"V"                         { PUT_STOKEN() return 'V'; }
<PATH_ATTRIBUTE>"z"                         { PUT_STOKEN() return 'Z'; }
<PATH_ATTRIBUTE>"Z"                         { PUT_STOKEN() return 'z'; }
<PATH_ATTRIBUTE>[+-]?([0-9]*[.])?[0-9]+     { PUT_STOKEN() return ATTRIBUTE_FLOAT_VALUE; }
<PATH_ATTRIBUTE>","                         { PUT_STOKEN() return ','; }
<PATH_ATTRIBUTE>"\""|"\'"                   { PUT_STOKEN() BEGIN(ATTRIBUTES); return ATTRIBUTE_QUOTE; }

<*>[\n]+              { nline += 1; printf("newline\n"); }
<*>[ \t]+             { printf("space\n"); }
<*>.                  { printf(" * lexer: unexpected character: line:%d: %c\n", nline, *yytext); return ERROR; }

%%
