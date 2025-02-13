%debug

%{

/* https://www.w3.org/TR/SVG2/paths.html is a good starting(and maybe the sufficient one) reference
 * to build a SVG yacc grammar.
 * https://www.w3.org/TR/SVG2/paths.html#PathDataBNF has a starting point to build a grammar.
*/

#include <stdio.h>
#include <stdlib.h>

#include "../aux_svg.h"

#ifdef __cplusplus
extern "C" {
#endif

int yy_svg_docu_error(const char *s);
int yy_svg_docu_lex();

extern FILE *yy_svg_docu_in;
extern char *yy_svg_docu_text;
extern int   yy_svg_docu_leng;

int aux_svg_parse_fn(const char *path);

#ifdef __cplusplus
}
#endif

%}

%union {
 struct {
    char    *ps;
    int      len;
 }sval;
}

%token TAG_E
%token OPEN_SVG CLOSE_SVG
%token OPEN_XML_DECL CLOSE_XML_DECL OPEN_COMMENTS CLOSE_COMMENTS
%token OPEN_DEFS
%token OPEN_G CLOSE_G
%token OPEN_PATH
%token OPEN_TAG CLOSE_TAG SELF_CLOSING_TAG
%token ATTRIBUTE_NM_WIDTH ATTRIBUTE_NM_HEIGHT ATTRIBUTE_NM_VIEWBOX ATTRIBUTE_NM_VERSION ATTRIBUTE_NM_ID ATTRIBUTE_NM_XMLNS ATTRIBUTE_NM_XMLNS_SVG ATTRIBUTE_NM_STYLE ATTRIBUTE_NM_D
%token ATTRIBUTE_NM_ENCODING ATTRIBUTE_NM_STANDALONE
%token ATTRIBUTE_QUOTE ATTRIBUTE_NM_ERROR ATTRIBUTE_FLOAT_VALUE ATTRIBUTE_VERSION_STRING_VALUE ATTRIBUTE_STRING_VALUE

%token ERROR

%%

svg_document:
	svg_document_element_sequence
	;

svg_document_element_sequence:
	svg_document_element
	| svg_document_element svg_document_element_sequence
	;

svg_document_element:
	svg_comment
	| svg_xml_declaration
	| svg
	;

svg_comment:
	OPEN_COMMENTS CLOSE_COMMENTS
	;

svg_xml_declaration:
	OPEN_XML_DECL xml_attribute_list CLOSE_XML_DECL
	;

svg:
	OPEN_SVG attribute_list TAG_E tag_list CLOSE_SVG { printf("svg\n"); }
    ;

tag_list:
    /* empty */
    | tag_list tag
    ;

defs:
	OPEN_DEFS attribute_list SELF_CLOSING_TAG { printf("defs\n"); }
	;

g:
	OPEN_G attribute_list TAG_E tag_list CLOSE_G { printf("g\n"); }
	;

path:
	OPEN_PATH attribute_list SELF_CLOSING_TAG { printf("path\n"); }
	;

tag:
	defs
	| g
	| path
	;

xml_attribute_list:
    /* empty */
    | xml_attribute xml_attribute_list
    ;

xml_attribute:
    ATTRIBUTE_NM_VERSION       '=' ATTRIBUTE_QUOTE ATTRIBUTE_VERSION_STRING_VALUE ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_ENCODING    '=' ATTRIBUTE_QUOTE ATTRIBUTE_STRING_VALUE ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_STANDALONE  '=' ATTRIBUTE_QUOTE ATTRIBUTE_STRING_VALUE ATTRIBUTE_QUOTE
	;

attribute_list:
    /* empty */
    | attribute attribute_list
    ;

attribute:
	ATTRIBUTE_NM_D            '=' ATTRIBUTE_QUOTE svg_path             ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_WIDTH      '=' ATTRIBUTE_QUOTE attribute_width      ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_HEIGHT     '=' ATTRIBUTE_QUOTE attribute_height     ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_VIEWBOX    '=' ATTRIBUTE_QUOTE attribute_viewbox    ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_VERSION    '=' ATTRIBUTE_QUOTE attribute_version    ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_ID         '=' ATTRIBUTE_QUOTE attribute_id         ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_XMLNS      '=' ATTRIBUTE_QUOTE attribute_xmlns      ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_XMLNS_SVG  '=' ATTRIBUTE_QUOTE attribute_xmlns_svg  ATTRIBUTE_QUOTE
	| ATTRIBUTE_NM_STYLE      '=' ATTRIBUTE_QUOTE attribute_style      ATTRIBUTE_QUOTE
	;

/* A path data segment (if there is one) must begin with a "moveto" command.
 * If a relative moveto (m) appears as the first element of the path, then it is treated as a pair of absolute coordinates.
*/
svg_path:
	/* empty */
	| { aux_svg_set_op_state(ST_MOVETO_START_PATH); } drawto_command_sequence { aux_svg_end_path(); }
	;

drawto_command_sequence:
	drawto_command
	| drawto_command drawto_command_sequence
	;

drawto_command:
	moveto
	| closepath
	| lineto
	;

moveto:
	  'm' coordinate_pair_sequence      { aux_svg_set_pe_state(ST_PEN_REL); aux_svg_moveto(); }
	| 'M' coordinate_pair_sequence      { aux_svg_set_pe_state(ST_PEN_ABS); aux_svg_moveto(); }
	;

closepath:
	  'z'     { aux_svg_closepath(); }
	| 'Z'     { aux_svg_closepath(); }
	;

lineto:
	lineto_xy
	| vertical_lineto
	| horizontal_lineto
	;

lineto_xy:
	{ aux_svg_set_op_state(ST_LINETO); }  'l' coordinate_pair_sequence { aux_svg_set_pe_state(ST_PEN_REL); aux_svg_lineto(); }
	{ aux_svg_set_op_state(ST_LINETO); }| 'L' coordinate_pair_sequence { aux_svg_set_pe_state(ST_PEN_ABS); aux_svg_lineto(); }
	;

vertical_lineto:
	{ aux_svg_set_op_state(ST_LINETO_V); }  'v' coordinate_sequence { aux_svg_set_pe_state(ST_PEN_REL); aux_svg_lineto(); }
	{ aux_svg_set_op_state(ST_LINETO_V); }| 'V' coordinate_sequence { aux_svg_set_pe_state(ST_PEN_ABS); aux_svg_lineto(); }
	;

horizontal_lineto:
	{ aux_svg_set_op_state(ST_LINETO_H); }  'h' coordinate_sequence { aux_svg_set_pe_state(ST_PEN_REL); aux_svg_lineto(); }
	{ aux_svg_set_op_state(ST_LINETO_H); }| 'H' coordinate_sequence { aux_svg_set_pe_state(ST_PEN_ABS); aux_svg_lineto(); }
	;

coordinate_sequence:
	/* empty */
	| coordinate coordinate_sequence { double v;
                                       v = strtod($1.sval.ps, NULL);
                                       switch(aux_svg_get_op_state()) {
                                        case ST_LINETO_V: aux_svg_push_coordinate(0.0 , v);              break;
                                        case ST_LINETO_H: aux_svg_push_coordinate(v   , 0.0);              break;
                                        default: yyerror("singe coordinate sequence, not handled state"); break;
                                       }
      }
	;

coordinate_pair_sequence:
	/* empty */
	| coordinate_pair coordinate_pair_sequence
	;

coordinate_pair:
	coordinate ',' coordinate { double x,y; x = strtod($1.sval.ps, NULL); y = strtod($3.sval.ps, NULL); aux_svg_push_coordinate(x, y); }
	;

coordinate:
	ATTRIBUTE_FLOAT_VALUE
	;

attribute_width:
	/* empty */
	| ATTRIBUTE_FLOAT_VALUE
	;

attribute_height:
	/* empty */
	| ATTRIBUTE_FLOAT_VALUE
	;

attribute_viewbox:
	/* empty */
	| ATTRIBUTE_FLOAT_VALUE ATTRIBUTE_FLOAT_VALUE ATTRIBUTE_FLOAT_VALUE ATTRIBUTE_FLOAT_VALUE
	;

attribute_version:
	/* empty */
	| ATTRIBUTE_VERSION_STRING_VALUE
	;

attribute_id:
	/* empty */
	| ATTRIBUTE_STRING_VALUE
	;

attribute_xmlns:
	/* empty */
	| ATTRIBUTE_STRING_VALUE
	;

attribute_xmlns_svg:
	/* empty */
	| ATTRIBUTE_STRING_VALUE
	;

attribute_style:
	/* empty */
	| ATTRIBUTE_STRING_VALUE
	;

%%

int aux_svg_parse_fn(const char *path) {
    int   res;
	FILE *file = NULL;

    file = fopen(path, "r");
    if (!file) {
        perror(" * error opening file: ");
        return 1;
    }

    yy_svg_docu_in    = file;  // Set the input stream to the opened file
	yy_svg_docu_debug = 1;
    res = yy_svg_docu_parse();  // Start parsing the input file

    fclose(file);  // Close the file after parsing is complete
    return res;
}

int yyerror(const char *s) {
    fprintf(stderr, " * parser: %s\n", s);
    return 1;
}
