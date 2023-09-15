/* f:\usr\bin\yacc -P console/yyparse.c -p CMD -D console/gram.h -o console/gram.cc console/gram.y */
#ifdef YYTRACE
#define YYDEBUG 1
#else
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#endif
/*
 * Portable way of defining ANSI C prototypes
 */
#ifndef YY_ARGS
#ifdef __STDC__
#define YY_ARGS(x)	x
#else
#define YY_ARGS(x)	()
#endif
#endif

#ifdef YACC_WINDOWS

#include <windows.h>

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling CMDparse must supply this!
 */

#ifdef STRICT
extern HINSTANCE hInst;	
#else
extern HANDLE hInst;	
#endif

#endif	/* YACC_WINDOWS */

#if YYDEBUG
typedef struct yyNamedType_tag {	/* Tokens */
	char	* name;		/* printable name */
	short	token;		/* token # */
	short	type;		/* token type */
} yyNamedType;
typedef struct yyTypedRules_tag {	/* Typed rule table */
	char	* name;		/* compressed rule string */
	short	type;		/* rule result type */
} yyTypedRules;

#endif

#line 1 "console/gram.y"

#include "console/console.h"
#include "console/ast.h"
#include <stdlib.h>
#include <stdio.h>
#include "console/consoleInternal.h"

#ifndef YYDEBUG
#define YYDEBUG
#endif

#define YYSSIZE 350

int outtext(char *fmt, ...);
extern int serrors;
#define nil 0
#undef YY_ARGS
#define YY_ARGS(x)	x
#line 20 "console/gram.y"

        /* Reserved Word Definitions */
#define rwDEFINE	257
#define rwENDDEF	258
#define rwDECLARE	259
#define rwBREAK	260
#define rwELSE	261
#define rwCONTINUE	262
#define rwGLOBAL	263
#define rwIF	264
#define rwNIL	265
#define rwRETURN	266
#define rwWHILE	267
#define rwENDIF	268
#define rwENDWHILE	269
#define rwENDFOR	270
#define rwDEFAULT	271
#define rwFOR	272
#define rwDATABLOCK	273
#define rwSWITCH	274
#define rwCASE	275
#define rwSWITCHSTR	276
#define rwCASEOR	277
#define rwPACKAGE	278
#define ILLEGAL_TOKEN	279
#line 30 "console/gram.y"

        /* Constants and Identifier Definitions */
#define CHRCONST	280
#define INTCONST	281
#define TTAG	282
#define VAR	283
#define IDENT	284
#define STRATOM	285
#define TAGATOM	286
#define FLTCONST	287
#line 42 "console/gram.y"

        /* Operator Definitions */
#define opMINUSMINUS	288
#define opPLUSPLUS	289
#define STMT_SEP	290
#define opSHL	291
#define opSHR	292
#define opPLASN	293
#define opMIASN	294
#define opMLASN	295
#define opDVASN	296
#define opMODASN	297
#define opANDASN	298
#define opXORASN	299
#define opORASN	300
#define opSLASN	301
#define opSRASN	302
#define opCAT	303
#define opEQ	304
#define opNE	305
#define opGE	306
#define opLE	307
#define opAND	308
#define opOR	309
#define opSTREQ	310
#define opCOLONCOLON	311
typedef union {
	char c;
	int i;
	const char *s;
   char *str;
	double f;
	StmtNode *stmt;
	ExprNode *expr;
   SlotAssignNode *slist;
   VarNode *var;
   SlotDecl slot;
   ObjectBlockDecl odcl;
   ObjectDeclNode *od;
   AssignDecl asn;
   IfStmtNode *ifnode;
} YYSTYPE;
#define opMDASN	312
#define opNDASN	313
#define opNTASN	314
#define opSTRNE	315
#define UNARY	316
extern int CMDchar, yyerrflag;
extern YYSTYPE CMDlval;
#if YYDEBUG
enum YY_Types { YY_t_NoneDefined, YY_t_i, YY_t_c, YY_t_s, YY_t_str, YY_t_f, YY_t_ifnode, YY_t_stmt, YY_t_expr, YY_t_od, YY_t_odcl, YY_t_slist, YY_t_slot, YY_t_var, YY_t_asn
};
#endif
#if YYDEBUG
yyTypedRules yyRules[] = {
	{ "&00: %36 &00",  0},
	{ "%36: %05",  0},
	{ "%05:",  7},
	{ "%05: %05 %04",  7},
	{ "%04: %10",  7},
	{ "%04: %07",  7},
	{ "%04: %06",  7},
	{ "%06: &23 &29 &49 %08 &50 &48",  7},
	{ "%08: %07",  7},
	{ "%08: %08 %07",  7},
	{ "%09:",  7},
	{ "%09: %09 %10",  7},
	{ "%10: %20",  7},
	{ "%10: %21",  7},
	{ "%10: %22",  7},
	{ "%10: %24",  7},
	{ "%10: %03",  7},
	{ "%10: &05 &48",  7},
	{ "%10: &07 &48",  7},
	{ "%10: &11 &48",  7},
	{ "%10: &11 %28 &48",  7},
	{ "%10: %32 &48",  7},
	{ "%10: &27 &39 %28 &48",  7},
	{ "%10: &27 &39 %28 &46 %28 &48",  7},
	{ "%07: &02 &29 &44 %34 &45 &49 %09 &50",  7},
	{ "%07: &02 &29 &78 &29 &44 %34 &45 &49 %09 &50",  7},
	{ "%34:",  13},
	{ "%34: %33",  13},
	{ "%33: &28",  13},
	{ "%33: %33 &46 &28",  13},
	{ "%24: &18 &29 &44 &29 %01 &45 &49 %29 &50 &48",  7},
	{ "%25: &04 %19 &44 %15 %01 %16 &45 &49 %27 &50",  9},
	{ "%25: &04 %19 &44 %15 %01 %16 &45",  9},
	{ "%01:",  3},
	{ "%01: &47 &29",  3},
	{ "%15:",  8},
	{ "%15: %28",  8},
	{ "%16:",  8},
	{ "%16: &46 %11",  8},
	{ "%27:",  10},
	{ "%27: %29",  10},
	{ "%27: %26",  10},
	{ "%27: %29 %26",  10},
	{ "%26: %25 &48",  9},
	{ "%26: %26 %25 &48",  9},
	{ "%23: &49 %09 &50",  7},
	{ "%23: %10",  7},
	{ "%03: &19 &44 %28 &45 &49 %02 &50",  7},
	{ "%03: &21 &44 %28 &45 &49 %02 &50",  7},
	{ "%02: &20 %18 &47 %09",  6},
	{ "%02: &20 %18 &47 %09 &16 &47 %09",  6},
	{ "%02: &20 %18 &47 %09 %02",  6},
	{ "%18: %28",  8},
	{ "%18: %18 &22 %28",  8},
	{ "%20: &09 &44 %28 &45 %23",  7},
	{ "%20: &09 &44 %28 &45 %23 &06 %23",  7},
	{ "%21: &12 &44 %28 &45 %23",  7},
	{ "%22: &17 &44 %28 &48 %28 &48 %28 &45 %23",  7},
	{ "%32: %17",  7},
	{ "%28: %17",  8},
	{ "%28: &44 %28 &45",  8},
	{ "%28: %28 &51 %28",  8},
	{ "%28: %28 &43 %28",  8},
	{ "%28: %28 &42 %28",  8},
	{ "%28: %28 &41 %28",  8},
	{ "%28: %28 &33 %28",  8},
	{ "%28: %28 &34 %28",  8},
	{ "%28: %28 &35 %28",  8},
	{ "%28: %28 &36 %28",  8},
	{ "%28: &34 %28",  8},
	{ "%28: &35 %28",  8},
	{ "%28: &27",  8},
	{ "%28: %28 &83 %28 &47 %28",  8},
	{ "%28: %28 &37 %28",  8},
	{ "%28: %28 &38 %28",  8},
	{ "%28: %28 &73 %28",  8},
	{ "%28: %28 &74 %28",  8},
	{ "%28: %28 &71 %28",  8},
	{ "%28: %28 &72 %28",  8},
	{ "%28: %28 &76 %28",  8},
	{ "%28: %28 &58 %28",  8},
	{ "%28: %28 &59 %28",  8},
	{ "%28: %28 &75 %28",  8},
	{ "%28: %28 &77 %28",  8},
	{ "%28: %28 &84 %28",  8},
	{ "%28: %28 &54 %28",  8},
	{ "%28: &53 %28",  8},
	{ "%28: &52 %28",  8},
	{ "%28: &31",  8},
	{ "%28: &32",  8},
	{ "%28: &26",  8},
	{ "%28: &05",  8},
	{ "%28: %31",  8},
	{ "%28: &29",  8},
	{ "%28: &30",  8},
	{ "%28: &28",  8},
	{ "%28: &28 &79 %13 &86",  8},
	{ "%31: %28 &40 &29",  12},
	{ "%31: %28 &40 &29 &79 %13 &86",  12},
	{ "%19: &29",  8},
	{ "%19: &44 %28 &45",  8},
	{ "%35: &56",  14},
	{ "%35: &55",  14},
	{ "%35: &60 %28",  14},
	{ "%35: &61 %28",  14},
	{ "%35: &62 %28",  14},
	{ "%35: &63 %28",  14},
	{ "%35: &64 %28",  14},
	{ "%35: &65 %28",  14},
	{ "%35: &66 %28",  14},
	{ "%35: &67 %28",  14},
	{ "%35: &68 %28",  14},
	{ "%35: &69 %28",  14},
	{ "%17: %14",  8},
	{ "%17: %25",  8},
	{ "%17: &28 &39 %28",  8},
	{ "%17: &28 &79 %13 &86 &39 %28",  8},
	{ "%17: &28 %35",  8},
	{ "%17: &28 &79 %13 &86 %35",  8},
	{ "%17: %31 %35",  8},
	{ "%17: %31 &39 %28",  8},
	{ "%17: %31 &39 &49 %11 &50",  8},
	{ "%14: &29 &44 %12 &45",  8},
	{ "%14: &29 &78 &29 &44 %12 &45",  8},
	{ "%14: %28 &40 &29 &44 %12 &45",  8},
	{ "%12:",  8},
	{ "%12: %11",  8},
	{ "%11: %28",  8},
	{ "%11: %11 &46 %28",  8},
	{ "%29: %30",  11},
	{ "%29: %29 %30",  11},
	{ "%30: &29 &39 %28 &48",  11},
	{ "%30: &18 &39 %28 &48",  11},
	{ "%30: &29 &79 %13 &86 &39 %28 &48",  11},
	{ "%13: %28",  8},
	{ "%13: %13 &46 %28",  8},
{ "$accept",  0},{ "error",  0}
};
yyNamedType yyTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "rwDEFINE",  257,  1},
	{ "rwENDDEF",  258,  1},
	{ "rwDECLARE",  259,  1},
	{ "rwBREAK",  260,  1},
	{ "rwELSE",  261,  1},
	{ "rwCONTINUE",  262,  1},
	{ "rwGLOBAL",  263,  1},
	{ "rwIF",  264,  1},
	{ "rwNIL",  265,  1},
	{ "rwRETURN",  266,  1},
	{ "rwWHILE",  267,  1},
	{ "rwENDIF",  268,  1},
	{ "rwENDWHILE",  269,  1},
	{ "rwENDFOR",  270,  1},
	{ "rwDEFAULT",  271,  1},
	{ "rwFOR",  272,  1},
	{ "rwDATABLOCK",  273,  1},
	{ "rwSWITCH",  274,  1},
	{ "rwCASE",  275,  1},
	{ "rwSWITCHSTR",  276,  1},
	{ "rwCASEOR",  277,  1},
	{ "rwPACKAGE",  278,  1},
	{ "ILLEGAL_TOKEN",  279,  0},
	{ "CHRCONST",  280,  2},
	{ "INTCONST",  281,  1},
	{ "TTAG",  282,  3},
	{ "VAR",  283,  3},
	{ "IDENT",  284,  3},
	{ "STRATOM",  285,  4},
	{ "TAGATOM",  286,  4},
	{ "FLTCONST",  287,  5},
	{ "'+'",  43,  1},
	{ "'-'",  45,  1},
	{ "'*'",  42,  1},
	{ "'/'",  47,  1},
	{ "'<'",  60,  1},
	{ "'>'",  62,  1},
	{ "'='",  61,  1},
	{ "'.'",  46,  1},
	{ "'|'",  124,  1},
	{ "'&'",  38,  1},
	{ "'%'",  37,  1},
	{ "'('",  40,  1},
	{ "')'",  41,  1},
	{ "','",  44,  1},
	{ "':'",  58,  1},
	{ "';'",  59,  1},
	{ "'{'",  123,  1},
	{ "'}'",  125,  1},
	{ "'^'",  94,  1},
	{ "'~'",  126,  1},
	{ "'!'",  33,  1},
	{ "'@'",  64,  1},
	{ "opMINUSMINUS",  288,  1},
	{ "opPLUSPLUS",  289,  1},
	{ "STMT_SEP",  290,  1},
	{ "opSHL",  291,  1},
	{ "opSHR",  292,  1},
	{ "opPLASN",  293,  1},
	{ "opMIASN",  294,  1},
	{ "opMLASN",  295,  1},
	{ "opDVASN",  296,  1},
	{ "opMODASN",  297,  1},
	{ "opANDASN",  298,  1},
	{ "opXORASN",  299,  1},
	{ "opORASN",  300,  1},
	{ "opSLASN",  301,  1},
	{ "opSRASN",  302,  1},
	{ "opCAT",  303,  1},
	{ "opEQ",  304,  1},
	{ "opNE",  305,  1},
	{ "opGE",  306,  1},
	{ "opLE",  307,  1},
	{ "opAND",  308,  1},
	{ "opOR",  309,  1},
	{ "opSTREQ",  310,  1},
	{ "opCOLONCOLON",  311,  1},
	{ "'['",  91,  0},
	{ "opMDASN",  312,  0},
	{ "opNDASN",  313,  0},
	{ "opNTASN",  314,  0},
	{ "'?'",  63,  0},
	{ "opSTRNE",  315,  0},
	{ "UNARY",  316,  0},
	{ "']'",  93,  0}

};
#endif
static short yydef[] = {

	   3, 65535,   50,   49,   48, 65531,   96,   92, 65527,   47, 
	  46,   31,   30,   63,   62,   61,   60,   59,   58,   57, 
	  56,   55,   54,   53,   70,   67,   68,   65,   52,   45, 
	  44,   43,   42,   41,   40,   39,   38,   37,   36,   35, 
	  34,   33,   29,   28,   27,   26,   25,   24,   23,   22, 
	65523, 65515,   51, 65511, 65507,    9, 65503, 65497,    5,   71, 
	  69,   32,   20, 65493, 65489,   64,  124,   18,   11,    6, 
	 124, 65485,  124,   19,   16,   14,   13,   15,  124,   17
};
static short yyex[] = {

	   0,    0, 65535,    1,   59,   21, 65535,   98,   41,   66, 
	65535,    1,   41,    8,   44,    8,   58,    8, 65535,    1, 
	  41,    4, 65535,    1,   41,   66, 65535,    1,   41,   66, 
	65535,    1,   41,    7,   44,    7, 65535,    1,   41,    7, 
	65535,    1,   41,   10, 65535,    1,   41,    4, 65535,    1, 
	 125,   12, 65535,    1
};
static short yyact[] = {

	65312, 65316, 65313, 65314, 65311, 65324, 65322, 65528, 65327, 65319, 
	65326, 65318, 65317, 65323, 65321, 65320, 65328, 65289, 65529, 65533, 
	65532, 65291, 65287, 65288,  287,  286,  285,  284,  283,  282, 
	 281,  278,  276,  274,  273,  272,  267,  266,  264,  262, 
	 260,  259,  257,  126,   45,   42,   40,   33, 65329, 65340, 
	65296, 65295, 65339, 65338, 65337, 65336, 65335, 65334, 65333, 65332, 
	65331, 65330,  302,  301,  300,  299,  298,  297,  296,  295, 
	 294,  293,  289,  288,   91,   61, 65527, 65341,  311,   40, 
	65342, 65296, 65295, 65339, 65338, 65337, 65336, 65335, 65334, 65333, 
	65332, 65331, 65330,  302,  301,  300,  299,  298,  297,  296, 
	 295,  294,  293,  289,  288,   61, 65312, 65316, 65313, 65314, 
	65311, 65322, 65290, 65289, 65286, 65533, 65532, 65291, 65287, 65288, 
	 287,  286,  285,  284,  283,  282,  281,  260,  259,  126, 
	  45,   42,   40,   33, 65364, 65363, 65359, 65361, 65360, 65343, 
	65358, 65356, 65355, 65357, 65344, 65365, 65362, 65349, 65348, 65352, 
	65351, 65354, 65353, 65347, 65350, 65346, 65345,  315,  310,  309, 
	 308,  307,  306,  305,  304,  292,  291,  124,   94,   64, 
	  63,   62,   60,   47,   46,   45,   43,   42,   38,   37, 
	65367,   40, 65368,   40, 65369,   40, 65370,   40, 65371,   40, 
	65372, 65293,  284,   40, 65374,  284, 65375,  284, 65376,   61, 
	65264,   59, 65312, 65316, 65313, 65314, 65262, 65311, 65322, 65290, 
	65289, 65286, 65533, 65532, 65291, 65287, 65288,  287,  286,  285, 
	 284,  283,  282,  281,  260,  259,  126,   59,   45,   42, 
	  40,   33, 65261,   59, 65260,   59, 65378,  284, 65380,  284, 
	65312, 65316, 65313, 65314, 65382, 65311, 65322, 65290, 65289, 65286, 
	65533, 65532, 65291, 65287, 65288,  287,  286,  285,  284,  283, 
	 282,  281,  260,  259,  126,  123,   45,   42,   40,   33, 
	65343,   46, 65507,  284, 65364, 65363, 65285, 65359, 65361, 65360, 
	65343, 65358, 65356, 65355, 65357, 65344, 65365, 65362, 65349, 65348, 
	65352, 65351, 65354, 65353, 65347, 65350, 65346, 65345,  315,  310, 
	 309,  308,  307,  306,  305,  304,  292,  291,  124,   94, 
	  64,   63,   62,   60,   47,   46,   45,   43,   42,   41, 
	  38,   37, 65485,   40, 65390,   40, 65484, 65391,  311,   40, 
	65364, 65363, 65359, 65361, 65360, 65343, 65358, 65263, 65356, 65355, 
	65357, 65344, 65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 
	65347, 65350, 65346, 65345,  315,  310,  309,  308,  307,  306, 
	 305,  304,  292,  291,  124,   94,   64,   63,   62,   60, 
	  59,   47,   46,   45,   43,   42,   38,   37, 65393,  123, 
	65394, 65483,   93,   44, 65482,   40, 65395,   44, 65303,   41, 
	65481, 65397,   91,   40, 65364, 65359, 65361, 65360, 65343, 65358, 
	65349, 65348,  292,  291,   47,   46,   45,   43,   42,   37, 
	65364, 65363, 65359, 65361, 65360, 65343, 65358, 65356, 65355, 65344, 
	65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 65346, 65345, 
	 315,  310,  307,  306,  305,  304,  292,  291,  124,   94, 
	  64,   62,   60,   47,   46,   45,   43,   42,   38,   37, 
	65364, 65359, 65361, 65360, 65343, 65358,   47,   46,   45,   43, 
	  42,   37, 65364, 65363, 65359, 65361, 65360, 65343, 65358, 65356, 
	65355, 65344, 65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 
	65347, 65346, 65345,  315,  310,  308,  307,  306,  305,  304, 
	 292,  291,  124,   94,   64,   62,   60,   47,   46,   45, 
	  43,   42,   38,   37, 65364, 65359, 65361, 65360, 65343, 65358, 
	65356, 65355, 65344, 65349, 65348, 65354, 65353, 65346, 65345,  315, 
	 310,  307,  306,  292,  291,   64,   62,   60,   47,   46, 
	  45,   43,   42,   37, 65364, 65359, 65361, 65360, 65343, 65358, 
	65344, 65349, 65348, 65346, 65345,  315,  310,  292,  291,   64, 
	  47,   46,   45,   43,   42,   37, 65364, 65363, 65359, 65361, 
	65360, 65343, 65358, 65398, 65356, 65355, 65357, 65344, 65365, 65362, 
	65349, 65348, 65352, 65351, 65354, 65353, 65347, 65350, 65346, 65345, 
	 315,  310,  309,  308,  307,  306,  305,  304,  292,  291, 
	 124,   94,   64,   63,   62,   60,   58,   47,   46,   45, 
	  43,   42,   38,   37, 65364, 65359, 65343, 65358,   47,   46, 
	  42,   37, 65364, 65363, 65359, 65361, 65360, 65343, 65358, 65356, 
	65355, 65344, 65365, 65349, 65348, 65352, 65351, 65354, 65353, 65346, 
	65345,  315,  310,  307,  306,  305,  304,  292,  291,   94, 
	  64,   62,   60,   47,   46,   45,   43,   42,   38,   37, 
	65364, 65359, 65361, 65360, 65343, 65358, 65356, 65355, 65344, 65349, 
	65348, 65352, 65351, 65354, 65353, 65346, 65345,  315,  310,  307, 
	 306,  305,  304,  292,  291,   64,   62,   60,   47,   46, 
	  45,   43,   42,   37, 65364, 65363, 65359, 65361, 65360, 65343, 
	65358, 65356, 65355, 65344, 65349, 65348, 65352, 65351, 65354, 65353, 
	65346, 65345,  315,  310,  307,  306,  305,  304,  292,  291, 
	  64,   62,   60,   47,   46,   45,   43,   42,   38,   37, 
	65364, 65363, 65359, 65361, 65360, 65343, 65358, 65399, 65356, 65355, 
	65357, 65344, 65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 
	65347, 65350, 65346, 65345,  315,  310,  309,  308,  307,  306, 
	 305,  304,  292,  291,  124,   94,   64,   63,   62,   60, 
	  59,   47,   46,   45,   43,   42,   38,   37, 65364, 65363, 
	65400, 65359, 65361, 65360, 65343, 65358, 65356, 65355, 65357, 65344, 
	65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 65347, 65350, 
	65346, 65345,  315,  310,  309,  308,  307,  306,  305,  304, 
	 292,  291,  124,   94,   64,   63,   62,   60,   47,   46, 
	  45,   43,   42,   41,   38,   37, 65364, 65363, 65401, 65359, 
	65361, 65360, 65343, 65358, 65356, 65355, 65357, 65344, 65365, 65362, 
	65349, 65348, 65352, 65351, 65354, 65353, 65347, 65350, 65346, 65345, 
	 315,  310,  309,  308,  307,  306,  305,  304,  292,  291, 
	 124,   94,   64,   63,   62,   60,   47,   46,   45,   43, 
	  42,   41,   38,   37, 65364, 65363, 65402, 65359, 65361, 65360, 
	65343, 65358, 65356, 65355, 65357, 65344, 65365, 65362, 65349, 65348, 
	65352, 65351, 65354, 65353, 65347, 65350, 65346, 65345,  315,  310, 
	 309,  308,  307,  306,  305,  304,  292,  291,  124,   94, 
	  64,   63,   62,   60,   47,   46,   45,   43,   42,   41, 
	  38,   37, 65364, 65363, 65403, 65359, 65361, 65360, 65343, 65358, 
	65356, 65355, 65357, 65344, 65365, 65362, 65349, 65348, 65352, 65351, 
	65354, 65353, 65347, 65350, 65346, 65345,  315,  310,  309,  308, 
	 307,  306,  305,  304,  292,  291,  124,   94,   64,   63, 
	  62,   60,   47,   46,   45,   43,   42,   41,   38,   37, 
	65364, 65363, 65294, 65359, 65361, 65360, 65343, 65358, 65356, 65355, 
	65357, 65344, 65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 
	65347, 65350, 65346, 65345,  315,  310,  309,  308,  307,  306, 
	 305,  304,  292,  291,  124,   94,   64,   63,   62,   60, 
	  47,   46,   45,   43,   42,   41,   38,   37, 65478,  284, 
	65404,  284, 65269,  283, 65364, 65363, 65359, 65361, 65406, 65360, 
	65343, 65358, 65265, 65356, 65355, 65357, 65344, 65365, 65362, 65349, 
	65348, 65352, 65351, 65354, 65353, 65347, 65350, 65346, 65345,  315, 
	 310,  309,  308,  307,  306,  305,  304,  292,  291,  124, 
	  94,   64,   63,   62,   60,   59,   47,   46,   45,   44, 
	  43,   42,   38,   37, 65324,  257, 65408, 65296, 65295, 65339, 
	65338, 65337, 65336, 65335, 65334, 65333, 65332, 65331, 65330,  302, 
	 301,  300,  299,  298,  297,  296,  295,  294,  293,  289, 
	 288,   61, 65395, 65302,  125,   44, 65312, 65316, 65313, 65314, 
	65258, 65311, 65322, 65528, 65327, 65319, 65326, 65318, 65317, 65323, 
	65321, 65320, 65289, 65529, 65533, 65532, 65291, 65287, 65288,  287, 
	 286,  285,  284,  283,  282,  281,  276,  274,  273,  272, 
	 267,  266,  264,  262,  260,  259,  126,  123,   45,   42, 
	  40,   33, 65413,  123, 65414,  123, 65415,   58, 65471,   40, 
	65417,   44, 65418,   41, 65420, 65324,  257,  125, 65304,   41, 
	65305,   41, 65394, 65292,   93,   44, 65364, 65363, 65359, 65361, 
	65360, 65343, 65358, 65356, 65355, 65344, 65365, 65362, 65349, 65348, 
	65352, 65351, 65354, 65353, 65347, 65350, 65346, 65345,  315,  310, 
	 309,  308,  307,  306,  305,  304,  292,  291,  124,   94, 
	  64,   62,   60,   47,   46,   45,   43,   42,   38,   37, 
	65364, 65363, 65359, 65361, 65360, 65343, 65358, 65421, 65356, 65355, 
	65357, 65344, 65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 
	65347, 65350, 65346, 65345,  315,  310,  309,  308,  307,  306, 
	 305,  304,  292,  291,  124,   94,   64,   63,   62,   60, 
	  59,   47,   46,   45,   43,   42,   38,   37, 65423,  261, 
	65424,  275, 65273,  284, 65427,   44, 65429,   41, 65270,  283, 
	65469,  123, 65364, 65363, 65359, 65361, 65360, 65343, 65358, 65266, 
	65356, 65355, 65357, 65344, 65365, 65362, 65349, 65348, 65352, 65351, 
	65354, 65353, 65347, 65350, 65346, 65345,  315,  310,  309,  308, 
	 307,  306,  305,  304,  292,  291,  124,   94,   64,   63, 
	  62,   60,   59,   47,   46,   45,   43,   42,   38,   37, 
	65255,   59, 65312, 65316, 65313, 65314, 65276, 65311, 65322, 65528, 
	65327, 65319, 65326, 65318, 65317, 65323, 65321, 65320, 65289, 65529, 
	65533, 65532, 65291, 65287, 65288,  287,  286,  285,  284,  283, 
	 282,  281,  276,  274,  273,  272,  267,  266,  264,  262, 
	 260,  259,  126,  125,   45,   42,   40,   33, 65279,  125, 
	65278,  125, 65466,   41, 65433,  123, 65434,   41, 65364, 65363, 
	65436, 65359, 65361, 65360, 65343, 65358, 65356, 65355, 65357, 65344, 
	65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 65347, 65350, 
	65346, 65345,  315,  310,  309,  308,  307,  306,  305,  304, 
	 292,  291,  124,   94,   64,   63,   62,   60,   47,   46, 
	  45,   43,   42,   41,   38,   37, 65465, 65437,  277,   58, 
	65464,  123, 65438, 65439,  284,  273, 65463,  123, 65312, 65316, 
	65313, 65314, 65267, 65311, 65322, 65528, 65327, 65319, 65326, 65318, 
	65317, 65323, 65321, 65320, 65289, 65529, 65533, 65532, 65291, 65287, 
	65288,  287,  286,  285,  284,  283,  282,  281,  276,  274, 
	 273,  272,  267,  266,  264,  262,  260,  259,  126,  125, 
	  45,   42,   40,   33, 65322, 65438, 65439,  284,  273,  259, 
	65443,   61, 65445, 65444,   91,   61, 65446, 65438, 65439,  284, 
	 273,  125, 65312, 65316, 65313, 65314, 65311, 65322, 65528, 65327, 
	65319, 65326, 65318, 65448, 65317, 65323, 65321, 65424, 65320, 65289, 
	65529, 65533, 65532, 65291, 65287, 65288,  287,  286,  285,  284, 
	 283,  282,  281,  276,  275,  274,  273,  272,  271,  267, 
	 266,  264,  262,  260,  259,  126,   45,   42,   40,   33, 
	65274,   59, 65322,  259, 65272,  125, 65271,   59, 65312, 65316, 
	65313, 65314, 65268, 65311, 65322, 65528, 65327, 65319, 65326, 65318, 
	65317, 65323, 65321, 65320, 65289, 65529, 65533, 65532, 65291, 65287, 
	65288,  287,  286,  285,  284,  283,  282,  281,  276,  274, 
	 273,  272,  267,  266,  264,  262,  260,  259,  126,  125, 
	  45,   42,   40,   33, 65457,   58, 65275,   59, 65364, 65363, 
	65359, 65361, 65360, 65343, 65358, 65309, 65356, 65355, 65357, 65344, 
	65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 65347, 65350, 
	65346, 65345,  315,  310,  309,  308,  307,  306,  305,  304, 
	 292,  291,  124,   94,   64,   63,   62,   60,   59,   47, 
	  46,   45,   43,   42,   38,   37, 65394, 65453,   93,   44, 
	65364, 65363, 65359, 65361, 65360, 65343, 65358, 65308, 65356, 65355, 
	65357, 65344, 65365, 65362, 65349, 65348, 65352, 65351, 65354, 65353, 
	65347, 65350, 65346, 65345,  315,  310,  309,  308,  307,  306, 
	 305,  304,  292,  291,  124,   94,   64,   63,   62,   60, 
	  59,   47,   46,   45,   43,   42,   38,   37, 65454,   61, 
	65312, 65316, 65313, 65314, 65311, 65322, 65528, 65327, 65319, 65326, 
	65318, 65317, 65323, 65321, 65320, 65289, 65529, 65533, 65532, 65291, 
	65287, 65288,  287,  286,  285,  284,  283,  282,  281,  276, 
	 274,  273,  272,  267,  266,  264,  262,  260,  259,  126, 
	  45,   42,   40,   33, 65364, 65363, 65359, 65361, 65360, 65343, 
	65358, 65310, 65356, 65355, 65357, 65344, 65365, 65362, 65349, 65348, 
	65352, 65351, 65354, 65353, 65347, 65350, 65346, 65345,  315,  310, 
	 309,  308,  307,  306,  305,  304,  292,  291,  124,   94, 
	  64,   63,   62,   60,   59,   47,   46,   45,   43,   42, 
	  38,   37,   -1
};
static short yypact[] = {

	  24,   48,   62,   78,   93,  180,  199,  235,  120,  271, 
	 271,  271,  271,  157,  157,  157,  157,  157,  157,  157, 
	 157,  157,  157,  157,  157,  387,  157,  157,  392,  402, 
	 402,  402,  430,  456,  456,  483,  519,  519,  545,  545, 
	 545,  545,  271,  271,  608,  608,  631,  667,  271,  702, 
	 120, 1013, 1079,  120,  120,  157, 1147, 1147, 1151,  157, 
	 157, 1188, 1259, 1265, 1013,  157, 1378,  157,  387, 1431, 
	1484, 1487, 1502,  157, 1526, 1553, 1487, 1553, 1708, 1732, 
	1778,  120, 1709, 1684, 1658, 1632, 1607, 1605, 1581, 1557, 
	 120,  120,  120, 1555, 1551, 1499, 1494, 1491,  120, 1119, 
	1461, 1437, 1434, 1428, 1402, 1377, 1375, 1373,  120, 1371, 
	1369,  120, 1119, 1345,  120, 1321, 1296, 1271, 1269, 1267, 
	1263, 1261, 1261, 1234, 1164, 1161, 1159,  120, 1156,  120, 
	1153, 1149, 1145, 1143, 1119, 1119,  120,  120,  120, 1094, 
	 120,  120, 1065, 1039, 1011, 1009,  984,  936,  888,  840, 
	 792,  744,  580,  120,  389,  385,  382,  379,  354,  120, 
	 328,  325,  323,  120,  120,  120,  120,  120,  120,  298, 
	 120,  120,  120,  120,  120,  120,  120,  120,  120,  120, 
	 120,  120,  120,  120,  120,  120,  120,  120,  120,  120, 
	 120,  120,  273,  255,  239,  120,  120,  120,  120,  120, 
	 120,  120,  120,  120,  120,  120,  120,  237,  233,  217, 
	 201,  197,  195,  192,  189,  187,  185,  183,  181,  120, 
	 157,  120,  120,  120,  120
};
static short yygo[] = {

	65416, 65472,   57, 65280, 65426, 65425,  121,   74, 65245, 65251, 
	65535, 65254, 65257, 65256, 65253,  142,  128, 65407, 65435, 65461, 
	65447, 65456, 65422,   78,   72,   70,   66, 65252, 65277, 65277, 
	65277, 65277, 65259,  135,  134,  112,   99,    0, 65467, 65396, 
	65510,  153,  108, 65409, 65410, 65381,   54,   53, 65451, 65411, 
	65379,  138,   91, 65297, 65479, 65428, 65530, 65530, 65530, 65530, 
	65530, 65530, 65530, 65530, 65530, 65530, 65284,  135,  134,  113, 
	 112,  100,   99,   88,   79,   74,    0, 65432, 65373, 65249, 
	65248, 65247, 65283, 65281, 65473, 65282,  134,  112,   99, 65246, 
	65441, 65449, 65441, 65449, 65298,   77,   76,   75,   71, 65458, 
	65460,   76, 65442, 65509, 65480, 65509, 65509, 65455, 65452, 65511, 
	65450, 65462, 65509, 65468, 65431, 65470, 65419, 65412, 65474, 65511, 
	65475, 65476, 65509, 65392, 65389, 65388, 65387, 65386, 65385, 65384, 
	65486, 65487, 65488, 65489, 65490, 65491, 65492, 65493, 65383, 65494, 
	65495, 65496, 65497, 65498, 65499, 65500, 65501, 65502, 65503, 65504, 
	65505, 65506, 65508, 65511, 65512, 65513, 65514, 65515, 65516, 65517, 
	65518, 65519, 65520, 65521, 65522, 65377, 65366, 65523, 65524, 65525, 
	65526, 65315,  224,  223,  222,  221,  219,  209,  206,  205, 
	 204,  203,  202,  201,  200,  199,  198,  197,  196,  195, 
	 193,  191,  190,  189,  188,  187,  186,  185,  184,  183, 
	 182,  181,  180,  179,  178,  177,  176,  175,  174,  173, 
	 172,  171,  170,  168,  167,  166,  165,  164,  163,  159, 
	 153,  141,  140,  138,  137,  136,  129,  127,  114,  111, 
	 108,   98,   92,   91,   90,   81,   54,   53,   50,    8, 
	65459, 65440,   71, 65307, 65307, 65306,   95,   76, 65531, 65325, 
	65477, 65430, 65405,   64, 65301, 65300, 65299,   52,    4, 65534,   -1
};
static short yypgo[] = {

	   0,    0,    0,  259,  252,  252,   94,    1,   54,   54, 
	  55,   55,  102,  102,  102,  102,    5,    5,   77,   77, 
	  79,  249,  171,  171,  171,  171,  171,  171,  171,  171, 
	 171,  171,  171,  171,  171,  171,  171,  171,  171,  171, 
	 171,  171,  171,  171,  171,  171,  171,  171,  171,  171, 
	 171,  171,  248,  256,  256,  256,  256,  256,  256,  256, 
	 256,  256,  256,   66,   66,   66,   45,   45,   40,   40, 
	  50,   50,  245,  245,  245,  241,  241,   53,   53,   53, 
	  66,   66,   66,   66,   66,   66,  256,  256,   78,   78, 
	 248,  171,  171,  171,  171,  171,  171,  171,  171,   81, 
	  80,   79,    5,    8,    8,   85,   85,  100,  100,    1, 
	  94,   89,  250,  250,   14,   14,   32,   32,   32,   32, 
	  32,   32,   32,   22,   22,   17,   17,   11,    9,    9, 
	   9,   10,   10,   32,   32,   32,   32,   32,    0
};
static short yyrlen[] = {

	   0,    0,    0,    1,    0,    1,    7,    0,    0,    1, 
	   0,    2,    0,    1,    1,    2,    4,    7,    1,    3, 
	   5,    1,    3,    3,    3,    3,    3,    3,    3,    3, 
	   2,    2,    5,    3,    3,    3,    3,    3,    3,    3, 
	   3,    3,    3,    3,    3,    3,    2,    2,    1,    1, 
	   1,    4,    3,    2,    2,    2,    2,    2,    2,    2, 
	   2,    2,    2,    3,    6,    3,    0,    1,    1,    3, 
	   1,    3,    7,    4,    4,    2,    1,    6,    6,    4, 
	   5,    2,    5,    2,    1,    1,    1,    1,    3,    1, 
	   6,    1,    1,    1,    1,    1,    1,    3,    1,    9, 
	   5,    7,    5,    7,    7,    1,    3,    3,    2,    2, 
	  10,   10,    3,    1,   10,    8,    6,    4,    2,    3, 
	   2,    2,    2,    2,    0,    2,    1,    6,    1,    1, 
	   1,    2,    0,    1,    1,    1,    1,    1,    2
};
#define YYS0	285
#define YYDELTA	153
#define YYNPACT	225
#define YYNDEF	80

#define YYr136	0
#define YYr137	1
#define YYr138	2
#define YYr1	3
#define YYr26	4
#define YYr27	5
#define YYr32	6
#define YYr33	7
#define YYr35	8
#define YYr36	9
#define YYr37	10
#define YYr38	11
#define YYr39	12
#define YYr40	13
#define YYr41	14
#define YYr42	15
#define YYr49	16
#define YYr50	17
#define YYr52	18
#define YYr53	19
#define YYr54	20
#define YYr58	21
#define YYr61	22
#define YYr62	23
#define YYr63	24
#define YYr64	25
#define YYr65	26
#define YYr66	27
#define YYr67	28
#define YYr68	29
#define YYr69	30
#define YYr70	31
#define YYr72	32
#define YYr73	33
#define YYr74	34
#define YYr75	35
#define YYr76	36
#define YYr77	37
#define YYr78	38
#define YYr79	39
#define YYr80	40
#define YYr81	41
#define YYr82	42
#define YYr83	43
#define YYr84	44
#define YYr85	45
#define YYr86	46
#define YYr87	47
#define YYr92	48
#define YYr93	49
#define YYr95	50
#define YYr96	51
#define YYr97	52
#define YYr103	53
#define YYr104	54
#define YYr105	55
#define YYr106	56
#define YYr107	57
#define YYr108	58
#define YYr109	59
#define YYr110	60
#define YYr111	61
#define YYr112	62
#define YYr115	63
#define YYr116	64
#define YYr120	65
#define YYr125	66
#define YYr126	67
#define YYr127	68
#define YYr128	69
#define YYr134	70
#define YYr135	71
#define YYr133	72
#define YYr132	73
#define YYr131	74
#define YYr130	75
#define YYr129	76
#define YYr124	77
#define YYr123	78
#define YYr122	79
#define YYr121	80
#define YYr119	81
#define YYr118	82
#define YYr117	83
#define YYr114	84
#define YYr113	85
#define YYr102	86
#define YYr101	87
#define YYr100	88
#define YYr99	89
#define YYr98	90
#define YYr94	91
#define YYr91	92
#define YYr90	93
#define YYr89	94
#define YYr88	95
#define YYr71	96
#define YYr60	97
#define YYr59	98
#define YYr57	99
#define YYr56	100
#define YYr55	101
#define YYr51	102
#define YYr48	103
#define YYr47	104
#define YYr46	105
#define YYr45	106
#define YYr44	107
#define YYr43	108
#define YYr34	109
#define YYr31	110
#define YYr30	111
#define YYr29	112
#define YYr28	113
#define YYr25	114
#define YYr24	115
#define YYr23	116
#define YYr22	117
#define YYr21	118
#define YYr20	119
#define YYr19	120
#define YYr18	121
#define YYr17	122
#define YYr11	123
#define YYr10	124
#define YYr9	125
#define YYr8	126
#define YYr7	127
#define YYr6	128
#define YYr5	129
#define YYr4	130
#define YYr3	131
#define YYr2	132
#define YYrACCEPT	YYr136
#define YYrERROR	YYr137
#define YYrLR2	YYr138
#if YYDEBUG
char * yysvar[] = {
	"$accept",
	"parent_block",
	"case_block",
	"switch_stmt",
	"decl",
	"decl_list",
	"package_decl",
	"fn_decl_stmt",
	"fn_decl_list",
	"statement_list",
	"stmt",
	"expr_list",
	"expr_list_decl",
	"aidx_expr",
	"funcall_expr",
	"object_name",
	"object_args",
	"stmt_expr",
	"case_expr",
	"class_name_expr",
	"if_stmt",
	"while_stmt",
	"for_stmt",
	"stmt_block",
	"datablock_decl",
	"object_decl",
	"object_decl_list",
	"object_declare_block",
	"expr",
	"slot_assign_list",
	"slot_assign",
	"slot_acc",
	"expression_stmt",
	"var_list",
	"var_list_decl",
	"assign_op_struct",
	"start",
	0
};
short yyrmap[] = {

	 136,  137,  138,    1,   26,   27,   32,   33,   35,   36, 
	  37,   38,   39,   40,   41,   42,   49,   50,   52,   53, 
	  54,   58,   61,   62,   63,   64,   65,   66,   67,   68, 
	  69,   70,   72,   73,   74,   75,   76,   77,   78,   79, 
	  80,   81,   82,   83,   84,   85,   86,   87,   92,   93, 
	  95,   96,   97,  103,  104,  105,  106,  107,  108,  109, 
	 110,  111,  112,  115,  116,  120,  125,  126,  127,  128, 
	 134,  135,  133,  132,  131,  130,  129,  124,  123,  122, 
	 121,  119,  118,  117,  114,  113,  102,  101,  100,   99, 
	  98,   94,   91,   90,   89,   88,   71,   60,   59,   57, 
	  56,   55,   51,   48,   47,   46,   45,   44,   43,   34, 
	  31,   30,   29,   28,   25,   24,   23,   22,   21,   20, 
	  19,   18,   17,   11,   10,    9,    8,    7,    6,    5, 
	   4,    3,    2,   12,   13,   14,   15,   16,    0
};
short yysmap[] = {

	   1,    2,    5,    7,    8,   18,   27,   31,   58,   63, 
	  65,   66,   67,  109,  110,  111,  112,  113,  114,  115, 
	 116,  117,  118,  119,  121,  123,  125,  127,  128,  129, 
	 130,  131,  132,  133,  134,  135,  136,  137,  138,  139, 
	 140,  141,  143,  144,  145,  146,  147,  148,  149,  150, 
	 158,  161,  166,  167,  171,  180,  181,  182,  185,  191, 
	 195,  199,  204,  208,  210,  216,  232,  239,  243,  244, 
	 250,  251,  256,  259,  260,  262,  263,  275,  282,  287, 
	 289,  288,  285,  279,  278,  277,  274,  272,  270,  269, 
	 267,  266,  265,  264,  261,  255,  253,  252,  249,  248, 
	 247,  246,  245,  240,  235,  230,  229,  228,  227,  225, 
	 224,  223,  222,  221,  220,  215,  213,  212,  211,  209, 
	 207,  206,  205,  200,  198,  197,  194,  193,  190,  187, 
	 186,  183,  178,  177,  176,  175,  174,  173,  172,  170, 
	 168,  165,  164,  162,  160,  159,  157,  156,  155,  154, 
	 153,  152,  142,  126,  124,  122,  120,  108,  104,  102, 
	 101,  100,   99,   97,   96,   95,   94,   93,   92,   91, 
	  90,   89,   88,   87,   86,   85,   84,   83,   82,   81, 
	  80,   79,   78,   77,   76,   75,   74,   73,   72,   71, 
	  70,   69,   68,   59,   57,   56,   53,   52,   51,   50, 
	  49,   48,   47,   46,   45,   44,   43,   37,   30,   29, 
	  28,   26,   25,   24,   23,   22,   21,   20,   19,   17, 
	  16,   15,   14,   13,   12,  290,  284,  286,  268,  254, 
	 218,  217,  169,  196,   60,  192,   42,    3,    4,   54, 
	  55,  179,   98,  219,    6,   61,    9,   10,   11,   62, 
	 151,   64,  258,  201,  238,  271,  241,  242,  202,  236, 
	 283,  273,  226,  276,  280,  231,  184,  281,  257,  233, 
	 188,  103,  163,  105,  106,  107,  237,  203,  214,  189, 
	 234,   38,   39,   40,   41,    0,   36,   35,   34,   33,   32
};
int yyntoken = 87;
int yynvar = 37;
int yynstate = 291;
int yynrule = 139;
#endif

#if YYDEBUG
/*
 * Package up YACC context for tracing
 */
typedef struct yyTraceItems_tag {
	int	state, lookahead, errflag, done;
	int	rule, npop;
	short	* states;
	int	nstates;
	YYSTYPE * values;
	int	nvalues;
	short	* types;
} yyTraceItems;
#endif

#line 2 "console/yyparse.c"

/*
 * Copyright 1985, 1990 by Mortice Kern Systems Inc.  All rights reserved.
 * 
 * Automaton to interpret LALR(1) tables.
 *
 * Macros:
 *	yyclearin - clear the lookahead token.
 *	yyerrok - forgive a pending error
 *	YYERROR - simulate an error
 *	YYACCEPT - halt and return 0
 *	YYABORT - halt and return 1
 *	YYRETURN(value) - halt and return value.  You should use this
 *		instead of return(value).
 *	YYREAD - ensure CMDchar contains a lookahead token by reading
 *		one if it does not.  See also YYSYNC.
 *	YYRECOVERING - 1 if syntax error detected and not recovered
 *		yet; otherwise, 0.
 *
 * Preprocessor flags:
 *	YYDEBUG - includes debug code if 1.  The parser will print
 *		 a travelogue of the parse if this is defined as 1
 *		 and CMDdebug is non-zero.
 *		yacc -t sets YYDEBUG to 1, but not CMDdebug.
 *	YYTRACE - turn on YYDEBUG, and undefine default trace functions
 *		so that the interactive functions in 'ytrack.c' will
 *		be used.
 *	YYSSIZE - size of state and value stacks (default 150).
 *	YYSTATIC - By default, the state stack is an automatic array.
 *		If this is defined, the stack will be static.
 *		In either case, the value stack is static.
 *	YYALLOC - Dynamically allocate both the state and value stacks
 *		by calling malloc() and free().
 *	YYDYNAMIC - Dynamically allocate (and reallocate, if necessary)
 *		both the state and value stacks by calling malloc(),
 *		realloc(), and free().
 *	YYSYNC - if defined, yacc guarantees to fetch a lookahead token
 *		before any action, even if it doesnt need it for a decision.
 *		If YYSYNC is defined, YYREAD will never be necessary unless
 *		the user explicitly sets CMDchar = -1
 *
 * Copyright (c) 1983, by the University of Waterloo
 */
/*
 * Prototypes
 */

extern int CMDlex YY_ARGS((void));
extern void CMDerror YY_ARGS((char *, ...));

#if YYDEBUG

#include <stdlib.h>		/* common prototypes */
#include <string.h>

extern char *	yyValue YY_ARGS((YYSTYPE, int));	/* print CMDlval */
extern void yyShowState YY_ARGS((yyTraceItems *));
extern void yyShowReduce YY_ARGS((yyTraceItems *));
extern void yyShowGoto YY_ARGS((yyTraceItems *));
extern void yyShowShift YY_ARGS((yyTraceItems *));
extern void yyShowErrRecovery YY_ARGS((yyTraceItems *));
extern void yyShowErrDiscard YY_ARGS((yyTraceItems *));

extern void yyShowRead YY_ARGS((int));
#endif

/*
 * If YYDEBUG defined and CMDdebug set,
 * tracing functions will be called at appropriate times in CMDparse()
 * Pass state of YACC parse, as filled into yyTraceItems yyx
 * If yyx.done is set by the tracing function, CMDparse() will terminate
 * with a return value of -1
 */
#define YY_TRACE(fn) { \
	yyx.state = yystate; yyx.lookahead = CMDchar; yyx.errflag =yyerrflag; \
	yyx.states = yys+1; yyx.nstates = yyps-yys; \
	yyx.values = yyv+1; yyx.nvalues = yypv-yyv; \
	yyx.types = yytypev+1; yyx.done = 0; \
	yyx.rule = yyi; yyx.npop = yyj; \
	fn(&yyx); \
	if (yyx.done) YYRETURN(-1); }

#ifndef I18N
#define m_textmsg(id, str, cls)	(str)
#else /*I18N*/
#include <m_nls.h>
#endif/*I18N*/

#ifndef YYSSIZE
# define YYSSIZE	150
#endif

#ifdef YYDYNAMIC
#define YYALLOC
char *getenv();
int atoi();
int yysinc = -1; /* stack size increment, <0 = double, 0 = none, >0 = fixed */
#endif

#ifdef YYALLOC
int yyssize = YYSSIZE;
#endif

#define YYERROR		goto yyerrlabel
#define yyerrok		yyerrflag = 0
#if YYDEBUG
#define yyclearin	{ if (CMDdebug) yyShowRead(-1); CMDchar = -1; }
#else
#define yyclearin	CMDchar = -1
#endif
#define YYACCEPT	YYRETURN(0)
#define YYABORT		YYRETURN(1)
#define YYRECOVERING()	(yyerrflag != 0)
#ifdef YYALLOC
#define YYRETURN(val)	{ retval = (val); goto yyReturn; }
#else
#define YYRETURN(val)	return(val);
#endif
#if YYDEBUG
/* The if..else makes this macro behave exactly like a statement */
# define YYREAD	if (CMDchar < 0) {					\
			if ((CMDchar = CMDlex()) < 0)	{		\
				if (CMDchar == -2) YYABORT; \
				CMDchar = 0;				\
			}	/* endif */			\
			if (CMDdebug)					\
				yyShowRead(CMDchar);			\
		} else
#else
# define YYREAD	if (CMDchar < 0) {					\
			if ((CMDchar = CMDlex()) < 0) {			\
				if (CMDchar == -2) YYABORT; \
				CMDchar = 0;				\
			}	/* endif */			\
		} else
#endif

#define YYERRCODE	1396787533		/* value of `error' */
#define YYTOKEN_BASE	256
#define	YYQYYP	yyq[yyq-yyp]

/*
 * Simulate bitwise negation as if was done on a two's complement machine.
 * This makes the generated code portable to machines with different
 * representations of integers (ie. signed magnitude).
 */
#define	yyneg(s)	(-((s)+1))

YYSTYPE	yyval;				/* $ */
YYSTYPE	*yypvt;				/* $n */
YYSTYPE	CMDlval;				/* CMDlex() sets this */

int	CMDchar,				/* current token */
	yyerrflag,			/* error flag */
	yynerrs;			/* error count */

#if YYDEBUG
int CMDdebug = 0;		/* debug if this flag is set */
extern char	*yysvar[];	/* table of non-terminals (aka 'variables') */
extern yyNamedType yyTokenTypes[];	/* table of terminals & their types */
extern short	yyrmap[], yysmap[];	/* map internal rule/states */
extern int	yynstate, yynvar, yyntoken, yynrule;

extern int	yyGetType YY_ARGS((int));	/* token type */
extern char	*yyptok YY_ARGS((int));	/* printable token string */
extern int	yyExpandName YY_ARGS((int, int, char *, int));
				  /* expand yyRules[] or yyStates[] */
static char *	yygetState YY_ARGS((int));

#define yyassert(condition, msg, arg) \
	if (!(condition)) { \
		printf(m_textmsg(2824, "\nyacc bug: ", "E")); \
		printf(msg, arg); \
		YYABORT; }
#else /* !YYDEBUG */
#define yyassert(condition, msg, arg)
#endif



#ifdef YACC_WINDOWS

/*
 * the following is the CMDparse() function that will be
 * callable by a windows type program. It in turn will
 * load all needed resources, obtain pointers to these
 * resources, and call a statically defined function
 * win_yyparse(), which is the original CMDparse() fn
 * When win_yyparse() is complete, it will return a
 * value to the new CMDparse(), where it will be stored
 * away temporarily, all resources will be freed, and
 * that return value will be given back to the caller
 * CMDparse(), as expected.
 */

static int win_yyparse();			/* prototype */

int CMDparse() 
{
	int wReturnValue;
	HANDLE hRes_table;		/* handle of resource after loading */
	short far *old_yydef;		/* the following are used for saving */
	short far *old_yyex;		/* the current pointers */
	short far *old_yyact;
	short far *old_yypact;
	short far *old_yygo;
	short far *old_yypgo;
	short far *old_yyrlen;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst, 
		FindResource (hInst, "UD_RES_yyYACC", "yyYACCTBL"));
	
	/*
	 * return an error code if any
	 * of the resources did not load
	 */

	if (hRes_table == NULL)
		return (1);
	
	/*
	 * the following code will lock the resources
	 * into fixed memory locations for the parser
	 * (also, save the current pointer values first)
	 */

	old_yydef = yydef;
	old_yyex = yyex;
	old_yyact = yyact;
	old_yypact = yypact;
	old_yygo = yygo;
	old_yypgo = yypgo;
	old_yyrlen = yyrlen;

	yydef = (short far *)LockResource (hRes_table);
	yyex = (short far *)(yydef + Sizeof_yydef);
	yyact = (short far *)(yyex + Sizeof_yyex);
	yypact = (short far *)(yyact + Sizeof_yyact);
	yygo = (short far *)(yypact + Sizeof_yypact);
	yypgo = (short far *)(yygo + Sizeof_yygo);
	yyrlen = (short far *)(yypgo + Sizeof_yypgo);

	/*
	 * call the official CMDparse() function
	 */

	wReturnValue = win_yyparse();

	/*
	 * unlock the resources
	 */

	UnlockResource (hRes_table);

	/*
	 * and now free the resource
	 */

	FreeResource (hRes_table);

	/*
	 * restore previous pointer values
	 */

	yydef = old_yydef;
	yyex = old_yyex;
	yyact = old_yyact;
	yypact = old_yypact;
	yygo = old_yygo;
	yypgo = old_yypgo;
	yyrlen = old_yyrlen;

	return (wReturnValue);
}	/* end CMDparse */

static int win_yyparse() 

#else /* YACC_WINDOWS */

/*
 * we are not compiling a windows resource
 * based parser, so call CMDparse() the old
 * standard way.
 */

int CMDparse() 

#endif /* YACC_WINDOWS */

{
#ifdef YACC_WINDOWS
	register short far	*yyp;	/* for table lookup */
	register short far	*yyq;
#else
	register short		*yyp;	/* for table lookup */
	register short		*yyq;
#endif	/* YACC_WINDOWS */
	register short		yyi;
	register short		*yyps;		/* top of state stack */
	register short		yystate;	/* current state */
	register YYSTYPE	*yypv;		/* top of value stack */
	register int		yyj;
#if YYDEBUG
	yyTraceItems	yyx;			/* trace block */
	short	* yytp;
	int	yyruletype = 0;
#endif
#ifdef YYSTATIC
	static short	yys[YYSSIZE + 1];
	static YYSTYPE	yyv[YYSSIZE + 1];
#if YYDEBUG
	static short	yytypev[YYSSIZE+1];	/* type assignments */
#endif
#else /* ! YYSTATIC */
#ifdef YYALLOC
	YYSTYPE *yyv;
	short	*yys;
#if YYDEBUG
	short	*yytypev;
#endif
	YYSTYPE save_yylval;
	YYSTYPE save_yyval;
	YYSTYPE *save_yypvt;
	int save_yychar, save_yyerrflag, save_yynerrs;
	int retval; 			/* return value holder */
#else
	short		yys[YYSSIZE + 1];
	static YYSTYPE	yyv[YYSSIZE + 1];	/* historically static */
#if YYDEBUG
	short	yytypev[YYSSIZE+1];		/* mirror type table */
#endif
#endif /* ! YYALLOC */
#endif /* ! YYSTATIC */
#ifdef YYDYNAMIC
	char *envp;
#endif


#ifdef YYDYNAMIC
	if ((envp = getenv("YYSTACKSIZE")) != (char *)0) {
		yyssize = atoi(envp);
		if (yyssize <= 0)
			yyssize = YYSSIZE;
	}
	if ((envp = getenv("YYSTACKINC")) != (char *)0)
		yysinc = atoi(envp);
#endif
#ifdef YYALLOC
	yys = (short *) malloc((yyssize + 1) * sizeof(short));
	yyv = (YYSTYPE *) malloc((yyssize + 1) * sizeof(YYSTYPE));
#if YYDEBUG
	yytypev = (short *) malloc((yyssize + 1) * sizeof(short));
#endif
	if (yys == (short *)0 || yyv == (YYSTYPE *)0
#if YYDEBUG
		|| yytypev == (short *) 0
#endif
	) {
		CMDerror(m_textmsg(4967, "Not enough space for parser stacks",
				  "E"));
		return 1;
	}
	save_yylval = CMDlval;
	save_yyval = yyval;
	save_yypvt = yypvt;
	save_yychar = CMDchar;
	save_yyerrflag = yyerrflag;
	save_yynerrs = yynerrs;
#endif

	yynerrs = 0;
	yyerrflag = 0;
	yyclearin;
	yyps = yys;
	yypv = yyv;
	*yyps = yystate = YYS0;		/* start state */
#if YYDEBUG
	yytp = yytypev;
	yyi = yyj = 0;			/* silence compiler warnings */
#endif

yyStack:
	yyassert((unsigned)yystate < yynstate, m_textmsg(587, "state %d\n", ""), yystate);
#ifdef YYDYNAMIC
	if (++yyps > &yys[yyssize]) {
		int yynewsize;
		int yysindex = yyps - yys;
		int yyvindex = yypv - yyv;
#if YYDEBUG
		int yytindex = yytp - yytypev;
#endif
		if (yysinc == 0) {		/* no increment */
			CMDerror(m_textmsg(4968, "Parser stack overflow", "E"));
			YYABORT;
		} else if (yysinc < 0)		/* binary-exponential */
			yynewsize = yyssize * 2;
		else				/* fixed increment */
			yynewsize = yyssize + yysinc;
		if (yynewsize < yyssize) {
			CMDerror(m_textmsg(4967,
					  "Not enough space for parser stacks",
					  "E"));
			YYABORT;
		}
		yyssize = yynewsize;
		yys = (short *) realloc(yys, (yyssize + 1) * sizeof(short));
		yyps = yys + yysindex;
		yyv = (YYSTYPE *) realloc(yyv, (yyssize + 1) * sizeof(YYSTYPE));
		yypv = yyv + yyvindex;
#if YYDEBUG
		yytypev = (short *)realloc(yytypev,(yyssize + 1)*sizeof(short));
		yytp = yytypev + yytindex;
#endif
		if (yys == (short *)0 || yyv == (YYSTYPE *)0
#if YYDEBUG
			|| yytypev == (short *) 0
#endif
		) {
			CMDerror(m_textmsg(4967, 
					  "Not enough space for parser stacks",
					  "E"));
			YYABORT;
		}
	}
#else
	if (++yyps > &yys[YYSSIZE]) {
		CMDerror(m_textmsg(4968, "Parser stack overflow", "E"));
		YYABORT;
	}
#endif /* !YYDYNAMIC */
	*yyps = yystate;	/* stack current state */
	*++yypv = yyval;	/* ... and value */
#if YYDEBUG
	*++yytp = yyruletype;	/* ... and type */

	if (CMDdebug)
		YY_TRACE(yyShowState)
#endif

	/*
	 *	Look up next action in action table.
	 */
yyEncore:
#ifdef YYSYNC
	YYREAD;
#endif

#ifdef YACC_WINDOWS
	if (yystate >= Sizeof_yypact) 	/* simple state */
#else /* YACC_WINDOWS */
	if (yystate >= sizeof yypact/sizeof yypact[0]) 	/* simple state */
#endif /* YACC_WINDOWS */
		yyi = yystate - YYDELTA;	/* reduce in any case */
	else {
		if(*(yyp = &yyact[yypact[yystate]]) >= 0) {
			/* Look for a shift on CMDchar */
#ifndef YYSYNC
			YYREAD;
#endif
			yyq = yyp;
			yyi = CMDchar;
			while (yyi < *yyp++)
				;
			if (yyi == yyp[-1]) {
				yystate = yyneg(YYQYYP);
#if YYDEBUG
				if (CMDdebug) {
					yyruletype = yyGetType(CMDchar);
					YY_TRACE(yyShowShift)
				}
#endif
				yyval = CMDlval;	/* stack what CMDlex() set */
				yyclearin;		/* clear token */
				if (yyerrflag)
					yyerrflag--;	/* successful shift */
				goto yyStack;
			}
		}

		/*
	 	 *	Fell through - take default action
	 	 */

#ifdef YACC_WINDOWS
		if (yystate >= Sizeof_yydef)
#else /* YACC_WINDOWS */
		if (yystate >= sizeof yydef /sizeof yydef[0])
#endif /* YACC_WINDOWS */
			goto yyError;
		if ((yyi = yydef[yystate]) < 0)	 { /* default == reduce? */
			/* Search exception table */
#ifdef YACC_WINDOWS
			yyassert((unsigned)yyneg(yyi) < Sizeof_yyex,
				m_textmsg(2825, "exception %d\n", "I num"), yystate);
#else /* YACC_WINDOWS */
			yyassert((unsigned)yyneg(yyi) < sizeof yyex/sizeof yyex[0],
				m_textmsg(2825, "exception %d\n", "I num"), yystate);
#endif /* YACC_WINDOWS */
			yyp = &yyex[yyneg(yyi)];
#ifndef YYSYNC
			YYREAD;
#endif
			while((yyi = *yyp) >= 0 && yyi != CMDchar)
				yyp += 2;
			yyi = yyp[1];
			yyassert(yyi >= 0,
				 m_textmsg(2826, "Ex table not reduce %d\n", "I num"), yyi);
		}
	}

	yyassert((unsigned)yyi < yynrule, m_textmsg(2827, "reduce %d\n", "I num"), yyi);
	yyj = yyrlen[yyi];
#if YYDEBUG
	if (CMDdebug)
		YY_TRACE(yyShowReduce)
	yytp -= yyj;
#endif
	yyps -= yyj;		/* pop stacks */
	yypvt = yypv;		/* save top */
	yypv -= yyj;
	yyval = yypv[1];	/* default action $ = $1 */
#if YYDEBUG
	yyruletype = yyRules[yyrmap[yyi]].type;
#endif

	switch (yyi) {		/* perform semantic action */
		
case YYr1: {	/* start :  decl_list */
#line 128 "console/gram.y"
 
} break;

case YYr2: {	/* decl_list :  */
#line 133 "console/gram.y"
 yyval.stmt = nil; 
} break;

case YYr3: {	/* decl_list :  decl_list decl */
#line 135 "console/gram.y"
 if(!statementList) { statementList = yypvt[0].stmt; } else { statementList->append(yypvt[0].stmt); } 
} break;

case YYr4: {	/* decl :  stmt */
#line 140 "console/gram.y"
 yyval.stmt = yypvt[0].stmt; 
} break;

case YYr5: {	/* decl :  fn_decl_stmt */
#line 142 "console/gram.y"
 yyval.stmt = yypvt[0].stmt; 
} break;

case YYr6: {	/* decl :  package_decl */
#line 144 "console/gram.y"
 yyval.stmt = yypvt[0].stmt; 
} break;

case YYr7: {	/* package_decl :  rwPACKAGE IDENT '{' fn_decl_list '}' ';' */
#line 149 "console/gram.y"
 yyval.stmt = yypvt[-2].stmt; for(StmtNode *walk = (yypvt[-2].stmt);walk;walk = walk->getNext() ) walk->setPackage(yypvt[-4].s); 
} break;

case YYr8: {	/* fn_decl_list :  fn_decl_stmt */
#line 154 "console/gram.y"
 yyval.stmt = yypvt[0].stmt; 
} break;

case YYr9: {	/* fn_decl_list :  fn_decl_list fn_decl_stmt */
#line 156 "console/gram.y"
 yyval.stmt = yypvt[-1].stmt; (yypvt[-1].stmt)->append(yypvt[0].stmt);  
} break;

case YYr10: {	/* statement_list :  */
#line 161 "console/gram.y"
 yyval.stmt = nil; 
} break;

case YYr11: {	/* statement_list :  statement_list stmt */
#line 163 "console/gram.y"
 if(!yypvt[-1].stmt) { yyval.stmt = yypvt[0].stmt; } else { (yypvt[-1].stmt)->append(yypvt[0].stmt); yyval.stmt = yypvt[-1].stmt; } 
} break;

case YYr17: {	/* stmt :  rwBREAK ';' */
#line 173 "console/gram.y"
 yyval.stmt = BreakStmtNode::alloc(); 
} break;

case YYr18: {	/* stmt :  rwCONTINUE ';' */
#line 175 "console/gram.y"
 yyval.stmt = ContinueStmtNode::alloc(); 
} break;

case YYr19: {	/* stmt :  rwRETURN ';' */
#line 177 "console/gram.y"
 yyval.stmt = ReturnStmtNode::alloc(NULL); 
} break;

case YYr20: {	/* stmt :  rwRETURN expr ';' */
#line 179 "console/gram.y"
 yyval.stmt = ReturnStmtNode::alloc(yypvt[-1].expr); 
} break;

case YYr21: {	/* stmt :  expression_stmt ';' */
#line 181 "console/gram.y"
 yyval.stmt = yypvt[-1].stmt; 
} break;

case YYr22: {	/* stmt :  TTAG '=' expr ';' */
#line 183 "console/gram.y"
 yyval.stmt = TTagSetStmtNode::alloc(yypvt[-3].s, yypvt[-1].expr, NULL); 
} break;

case YYr23: {	/* stmt :  TTAG '=' expr ',' expr ';' */
#line 185 "console/gram.y"
 yyval.stmt = TTagSetStmtNode::alloc(yypvt[-5].s, yypvt[-3].expr, yypvt[-1].expr); 
} break;

case YYr24: {	/* fn_decl_stmt :  rwDEFINE IDENT '(' var_list_decl ')' '{' statement_list '}' */
#line 190 "console/gram.y"
 yyval.stmt = FunctionDeclStmtNode::alloc(yypvt[-6].s, NULL, yypvt[-4].var, yypvt[-1].stmt); 
} break;

case YYr25: {	/* fn_decl_stmt :  rwDEFINE IDENT opCOLONCOLON IDENT '(' var_list_decl ')' '{' statement_list '}' */
#line 192 "console/gram.y"
 yyval.stmt = FunctionDeclStmtNode::alloc(yypvt[-6].s, yypvt[-8].s, yypvt[-4].var, yypvt[-1].stmt); 
} break;

case YYr26: {	/* var_list_decl :  */
#line 197 "console/gram.y"
 yyval.var = NULL; 
} break;

case YYr27: {	/* var_list_decl :  var_list */
#line 199 "console/gram.y"
 yyval.var = yypvt[0].var; 
} break;

case YYr28: {	/* var_list :  VAR */
#line 204 "console/gram.y"
 yyval.var = VarNode::alloc(yypvt[0].s, NULL); 
} break;

case YYr29: {	/* var_list :  var_list ',' VAR */
#line 206 "console/gram.y"
 yyval.var = yypvt[-2].var; ((StmtNode*)(yypvt[-2].var))->append((StmtNode*)VarNode::alloc(yypvt[0].s, NULL)); 
} break;

case YYr30: {	/* datablock_decl :  rwDATABLOCK IDENT '(' IDENT parent_block ')' '{' slot_assign_list '}' ';' */
#line 211 "console/gram.y"
 yyval.stmt = ObjectDeclNode::alloc(ConstantNode::alloc(yypvt[-8].s), ConstantNode::alloc(yypvt[-6].s), NULL, yypvt[-5].s, yypvt[-2].slist, NULL, true); 
} break;

case YYr31: {	/* object_decl :  rwDECLARE class_name_expr '(' object_name parent_block object_args ')' '{' object_declare_block '}' */
#line 216 "console/gram.y"
 yyval.od = ObjectDeclNode::alloc(yypvt[-8].expr, yypvt[-6].expr, yypvt[-4].expr, yypvt[-5].s, yypvt[-1].odcl.slots, yypvt[-1].odcl.decls, false); 
} break;

case YYr32: {	/* object_decl :  rwDECLARE class_name_expr '(' object_name parent_block object_args ')' */
#line 218 "console/gram.y"
 yyval.od = ObjectDeclNode::alloc(yypvt[-5].expr, yypvt[-3].expr, yypvt[-1].expr, yypvt[-2].s, NULL, NULL, false); 
} break;

case YYr33: {	/* parent_block :  */
#line 223 "console/gram.y"
 yyval.s = NULL; 
} break;

case YYr34: {	/* parent_block :  ':' IDENT */
#line 225 "console/gram.y"
 yyval.s = yypvt[0].s; 
} break;

case YYr35: {	/* object_name :  */
#line 230 "console/gram.y"
 yyval.expr = StrConstNode::alloc("", false); 
} break;

case YYr36: {	/* object_name :  expr */
#line 232 "console/gram.y"
 yyval.expr = yypvt[0].expr; 
} break;

case YYr37: {	/* object_args :  */
#line 237 "console/gram.y"
 yyval.expr = NULL; 
} break;

case YYr38: {	/* object_args :  ',' expr_list */
#line 239 "console/gram.y"
 yyval.expr = yypvt[0].expr; 
} break;

case YYr39: {	/* object_declare_block :  */
#line 244 "console/gram.y"
 yyval.odcl.slots = NULL; yyval.odcl.decls = NULL; 
} break;

case YYr40: {	/* object_declare_block :  slot_assign_list */
#line 246 "console/gram.y"
 yyval.odcl.slots = yypvt[0].slist; yyval.odcl.decls = NULL; 
} break;

case YYr41: {	/* object_declare_block :  object_decl_list */
#line 248 "console/gram.y"
 yyval.odcl.slots = NULL; yyval.odcl.decls = yypvt[0].od; 
} break;

case YYr42: {	/* object_declare_block :  slot_assign_list object_decl_list */
#line 250 "console/gram.y"
 yyval.odcl.slots = yypvt[-1].slist; yyval.odcl.decls = yypvt[0].od; 
} break;

case YYr43: {	/* object_decl_list :  object_decl ';' */
#line 255 "console/gram.y"
 yyval.od = yypvt[-1].od; 
} break;

case YYr44: {	/* object_decl_list :  object_decl_list object_decl ';' */
#line 257 "console/gram.y"
 yypvt[-2].od->append(yypvt[-1].od); yyval.od = yypvt[-2].od; 
} break;

case YYr45: {	/* stmt_block :  '{' statement_list '}' */
#line 262 "console/gram.y"
 yyval.stmt = yypvt[-1].stmt; 
} break;

case YYr46: {	/* stmt_block :  stmt */
#line 264 "console/gram.y"
 yyval.stmt = yypvt[0].stmt; 
} break;

case YYr47: {	/* switch_stmt :  rwSWITCH '(' expr ')' '{' case_block '}' */
#line 269 "console/gram.y"
 yyval.stmt = yypvt[-1].ifnode; yypvt[-1].ifnode->propagateSwitchExpr(yypvt[-4].expr, false); 
} break;

case YYr48: {	/* switch_stmt :  rwSWITCHSTR '(' expr ')' '{' case_block '}' */
#line 271 "console/gram.y"
 yyval.stmt = yypvt[-1].ifnode; yypvt[-1].ifnode->propagateSwitchExpr(yypvt[-4].expr, true); 
} break;

case YYr49: {	/* case_block :  rwCASE case_expr ':' statement_list */
#line 276 "console/gram.y"
 yyval.ifnode = IfStmtNode::alloc(yypvt[-3].i, yypvt[-2].expr, yypvt[0].stmt, NULL, false); 
} break;

case YYr50: {	/* case_block :  rwCASE case_expr ':' statement_list rwDEFAULT ':' statement_list */
#line 278 "console/gram.y"
 yyval.ifnode = IfStmtNode::alloc(yypvt[-6].i, yypvt[-5].expr, yypvt[-3].stmt, yypvt[0].stmt, false); 
} break;

case YYr51: {	/* case_block :  rwCASE case_expr ':' statement_list case_block */
#line 280 "console/gram.y"
 yyval.ifnode = IfStmtNode::alloc(yypvt[-4].i, yypvt[-3].expr, yypvt[-1].stmt, yypvt[0].ifnode, true); 
} break;

case YYr52: {	/* case_expr :  expr */
#line 285 "console/gram.y"
 yyval.expr = yypvt[0].expr;
} break;

case YYr53: {	/* case_expr :  case_expr rwCASEOR expr */
#line 287 "console/gram.y"
 (yypvt[-2].expr)->append(yypvt[0].expr); yyval.expr=yypvt[-2].expr; 
} break;

case YYr54: {	/* if_stmt :  rwIF '(' expr ')' stmt_block */
#line 292 "console/gram.y"
 yyval.stmt = IfStmtNode::alloc(yypvt[-4].i, yypvt[-2].expr, yypvt[0].stmt, NULL, false); 
} break;

case YYr55: {	/* if_stmt :  rwIF '(' expr ')' stmt_block rwELSE stmt_block */
#line 294 "console/gram.y"
 yyval.stmt = IfStmtNode::alloc(yypvt[-6].i, yypvt[-4].expr, yypvt[-2].stmt, yypvt[0].stmt, false); 
} break;

case YYr56: {	/* while_stmt :  rwWHILE '(' expr ')' stmt_block */
#line 299 "console/gram.y"
 yyval.stmt = LoopStmtNode::alloc(yypvt[-4].i, nil, yypvt[-2].expr, nil, yypvt[0].stmt, false); 
} break;

case YYr57: {	/* for_stmt :  rwFOR '(' expr ';' expr ';' expr ')' stmt_block */
#line 304 "console/gram.y"
 yyval.stmt = LoopStmtNode::alloc(yypvt[-8].i, yypvt[-6].expr, yypvt[-4].expr, yypvt[-2].expr, yypvt[0].stmt, false); 
} break;

case YYr58: {	/* expression_stmt :  stmt_expr */
#line 309 "console/gram.y"
 yyval.stmt = yypvt[0].expr; 
} break;

case YYr59: {	/* expr :  stmt_expr */
#line 314 "console/gram.y"
 yyval.expr = yypvt[0].expr; 
} break;

case YYr60: {	/* expr :  '(' expr ')' */
#line 316 "console/gram.y"
 yyval.expr = yypvt[-1].expr; 
} break;

case YYr61: {	/* expr :  expr '^' expr */
#line 318 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr62: {	/* expr :  expr '%' expr */
#line 320 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr63: {	/* expr :  expr '&' expr */
#line 322 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr64: {	/* expr :  expr '|' expr */
#line 324 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr65: {	/* expr :  expr '+' expr */
#line 326 "console/gram.y"
 yyval.expr = FloatBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr66: {	/* expr :  expr '-' expr */
#line 328 "console/gram.y"
 yyval.expr = FloatBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr67: {	/* expr :  expr '*' expr */
#line 330 "console/gram.y"
 yyval.expr = FloatBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr68: {	/* expr :  expr '/' expr */
#line 332 "console/gram.y"
 yyval.expr = FloatBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr69: {	/* expr :  '-' expr */
#line 334 "console/gram.y"
 yyval.expr = FloatUnaryExprNode::alloc(yypvt[-1].i, yypvt[0].expr); 
} break;

case YYr70: {	/* expr :  '*' expr */
#line 336 "console/gram.y"
 yyval.expr = TTagDerefNode::alloc(yypvt[0].expr); 
} break;

case YYr71: {	/* expr :  TTAG */
#line 338 "console/gram.y"
 yyval.expr = TTagExprNode::alloc(yypvt[0].s); 
} break;

case YYr72: {	/* expr :  expr '?' expr ':' expr */
#line 340 "console/gram.y"
 yyval.expr = ConditionalExprNode::alloc(yypvt[-4].expr, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr73: {	/* expr :  expr '<' expr */
#line 342 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr74: {	/* expr :  expr '>' expr */
#line 344 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr75: {	/* expr :  expr opGE expr */
#line 346 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr76: {	/* expr :  expr opLE expr */
#line 348 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr77: {	/* expr :  expr opEQ expr */
#line 350 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr78: {	/* expr :  expr opNE expr */
#line 352 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr79: {	/* expr :  expr opOR expr */
#line 354 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr80: {	/* expr :  expr opSHL expr */
#line 356 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr81: {	/* expr :  expr opSHR expr */
#line 358 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr82: {	/* expr :  expr opAND expr */
#line 360 "console/gram.y"
 yyval.expr = IntBinaryExprNode::alloc(yypvt[-1].i, yypvt[-2].expr, yypvt[0].expr); 
} break;

case YYr83: {	/* expr :  expr opSTREQ expr */
#line 362 "console/gram.y"
 yyval.expr = StreqExprNode::alloc(yypvt[-2].expr, yypvt[0].expr, true); 
} break;

case YYr84: {	/* expr :  expr opSTRNE expr */
#line 364 "console/gram.y"
 yyval.expr = StreqExprNode::alloc(yypvt[-2].expr, yypvt[0].expr, false); 
} break;

case YYr85: {	/* expr :  expr '@' expr */
#line 366 "console/gram.y"
 yyval.expr = StrcatExprNode::alloc(yypvt[-2].expr, yypvt[0].expr, yypvt[-1].i); 
} break;

case YYr86: {	/* expr :  '!' expr */
#line 368 "console/gram.y"
 yyval.expr = IntUnaryExprNode::alloc(yypvt[-1].i, yypvt[0].expr); 
} break;

case YYr87: {	/* expr :  '~' expr */
#line 370 "console/gram.y"
 yyval.expr = IntUnaryExprNode::alloc(yypvt[-1].i, yypvt[0].expr); 
} break;

case YYr88: {	/* expr :  TAGATOM */
#line 372 "console/gram.y"
 yyval.expr = StrConstNode::alloc(yypvt[0].str, true); 
} break;

case YYr89: {	/* expr :  FLTCONST */
#line 374 "console/gram.y"
 yyval.expr = FloatNode::alloc(yypvt[0].f); 
} break;

case YYr90: {	/* expr :  INTCONST */
#line 376 "console/gram.y"
 yyval.expr = IntNode::alloc(yypvt[0].i); 
} break;

case YYr91: {	/* expr :  rwBREAK */
#line 378 "console/gram.y"
 yyval.expr = ConstantNode::alloc(StringTable->insert("break")); 
} break;

case YYr92: {	/* expr :  slot_acc */
#line 380 "console/gram.y"
 yyval.expr = SlotAccessNode::alloc(yypvt[0].slot.object, yypvt[0].slot.array, yypvt[0].slot.slotName); 
} break;

case YYr93: {	/* expr :  IDENT */
#line 382 "console/gram.y"
 yyval.expr = ConstantNode::alloc(yypvt[0].s); 
} break;

case YYr94: {	/* expr :  STRATOM */
#line 384 "console/gram.y"
 yyval.expr = StrConstNode::alloc(yypvt[0].str, false); 
} break;

case YYr95: {	/* expr :  VAR */
#line 386 "console/gram.y"
 yyval.expr = (ExprNode*)VarNode::alloc(yypvt[0].s, NULL); 
} break;

case YYr96: {	/* expr :  VAR '[' aidx_expr ']' */
#line 388 "console/gram.y"
 yyval.expr = (ExprNode*)VarNode::alloc(yypvt[-3].s, yypvt[-1].expr); 
} break;

case YYr97: {	/* slot_acc :  expr '.' IDENT */
#line 393 "console/gram.y"
 yyval.slot.object = yypvt[-2].expr; yyval.slot.slotName = yypvt[0].s; yyval.slot.array = NULL; 
} break;

case YYr98: {	/* slot_acc :  expr '.' IDENT '[' aidx_expr ']' */
#line 395 "console/gram.y"
 yyval.slot.object = yypvt[-5].expr; yyval.slot.slotName = yypvt[-3].s; yyval.slot.array = yypvt[-1].expr; 
} break;

case YYr99: {	/* class_name_expr :  IDENT */
#line 400 "console/gram.y"
 yyval.expr = ConstantNode::alloc(yypvt[0].s); 
} break;

case YYr100: {	/* class_name_expr :  '(' expr ')' */
#line 402 "console/gram.y"
 yyval.expr = yypvt[-1].expr; 
} break;

case YYr101: {	/* assign_op_struct :  opPLUSPLUS */
#line 407 "console/gram.y"
 yyval.asn.token = '+'; yyval.asn.expr = FloatNode::alloc(1); 
} break;

case YYr102: {	/* assign_op_struct :  opMINUSMINUS */
#line 409 "console/gram.y"
 yyval.asn.token = '-'; yyval.asn.expr = FloatNode::alloc(1); 
} break;

case YYr103: {	/* assign_op_struct :  opPLASN expr */
#line 411 "console/gram.y"
 yyval.asn.token = '+'; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr104: {	/* assign_op_struct :  opMIASN expr */
#line 413 "console/gram.y"
 yyval.asn.token = '-'; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr105: {	/* assign_op_struct :  opMLASN expr */
#line 415 "console/gram.y"
 yyval.asn.token = '*'; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr106: {	/* assign_op_struct :  opDVASN expr */
#line 417 "console/gram.y"
 yyval.asn.token = '/'; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr107: {	/* assign_op_struct :  opMODASN expr */
#line 419 "console/gram.y"
 yyval.asn.token = '%'; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr108: {	/* assign_op_struct :  opANDASN expr */
#line 421 "console/gram.y"
 yyval.asn.token = '&'; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr109: {	/* assign_op_struct :  opXORASN expr */
#line 423 "console/gram.y"
 yyval.asn.token = '^'; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr110: {	/* assign_op_struct :  opORASN expr */
#line 425 "console/gram.y"
 yyval.asn.token = '|'; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr111: {	/* assign_op_struct :  opSLASN expr */
#line 427 "console/gram.y"
 yyval.asn.token = opSHL; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr112: {	/* assign_op_struct :  opSRASN expr */
#line 429 "console/gram.y"
 yyval.asn.token = opSHR; yyval.asn.expr = yypvt[0].expr; 
} break;

case YYr113: {	/* stmt_expr :  funcall_expr */
#line 434 "console/gram.y"
 yyval.expr = yypvt[0].expr; 
} break;

case YYr114: {	/* stmt_expr :  object_decl */
#line 436 "console/gram.y"
 yyval.expr = yypvt[0].od; 
} break;

case YYr115: {	/* stmt_expr :  VAR '=' expr */
#line 438 "console/gram.y"
 yyval.expr = AssignExprNode::alloc(yypvt[-2].s, NULL, yypvt[0].expr); 
} break;

case YYr116: {	/* stmt_expr :  VAR '[' aidx_expr ']' '=' expr */
#line 440 "console/gram.y"
 yyval.expr = AssignExprNode::alloc(yypvt[-5].s, yypvt[-3].expr, yypvt[0].expr); 
} break;

case YYr117: {	/* stmt_expr :  VAR assign_op_struct */
#line 442 "console/gram.y"
 yyval.expr = AssignOpExprNode::alloc(yypvt[-1].s, NULL, yypvt[0].asn.expr, yypvt[0].asn.token); 
} break;

case YYr118: {	/* stmt_expr :  VAR '[' aidx_expr ']' assign_op_struct */
#line 444 "console/gram.y"
 yyval.expr = AssignOpExprNode::alloc(yypvt[-4].s, yypvt[-2].expr, yypvt[0].asn.expr, yypvt[0].asn.token); 
} break;

case YYr119: {	/* stmt_expr :  slot_acc assign_op_struct */
#line 446 "console/gram.y"
 yyval.expr = SlotAssignOpNode::alloc(yypvt[-1].slot.object, yypvt[-1].slot.slotName, yypvt[-1].slot.array, yypvt[0].asn.token, yypvt[0].asn.expr); 
} break;

case YYr120: {	/* stmt_expr :  slot_acc '=' expr */
#line 448 "console/gram.y"
 yyval.expr = SlotAssignNode::alloc(yypvt[-2].slot.object, yypvt[-2].slot.array, yypvt[-2].slot.slotName, yypvt[0].expr); 
} break;

case YYr121: {	/* stmt_expr :  slot_acc '=' '{' expr_list '}' */
#line 450 "console/gram.y"
 yyval.expr = SlotAssignNode::alloc(yypvt[-4].slot.object, yypvt[-4].slot.array, yypvt[-4].slot.slotName, yypvt[-1].expr); 
} break;

case YYr122: {	/* funcall_expr :  IDENT '(' expr_list_decl ')' */
#line 455 "console/gram.y"
 yyval.expr = FuncCallExprNode::alloc(yypvt[-3].s, NULL, yypvt[-1].expr, false); 
} break;

case YYr123: {	/* funcall_expr :  IDENT opCOLONCOLON IDENT '(' expr_list_decl ')' */
#line 457 "console/gram.y"
 yyval.expr = FuncCallExprNode::alloc(yypvt[-3].s, yypvt[-5].s, yypvt[-1].expr, false); 
} break;

case YYr124: {	/* funcall_expr :  expr '.' IDENT '(' expr_list_decl ')' */
#line 459 "console/gram.y"
 yypvt[-5].expr->append(yypvt[-1].expr); yyval.expr = FuncCallExprNode::alloc(yypvt[-3].s, NULL, yypvt[-5].expr, true); 
} break;

case YYr125: {	/* expr_list_decl :  */
#line 464 "console/gram.y"
 yyval.expr = NULL; 
} break;

case YYr126: {	/* expr_list_decl :  expr_list */
#line 466 "console/gram.y"
 yyval.expr = yypvt[0].expr; 
} break;

case YYr127: {	/* expr_list :  expr */
#line 471 "console/gram.y"
 yyval.expr = yypvt[0].expr; 
} break;

case YYr128: {	/* expr_list :  expr_list ',' expr */
#line 473 "console/gram.y"
 (yypvt[-2].expr)->append(yypvt[0].expr); yyval.expr = yypvt[-2].expr; 
} break;

case YYr129: {	/* slot_assign_list :  slot_assign */
#line 478 "console/gram.y"
 yyval.slist = yypvt[0].slist; 
} break;

case YYr130: {	/* slot_assign_list :  slot_assign_list slot_assign */
#line 480 "console/gram.y"
 yypvt[-1].slist->append(yypvt[0].slist); yyval.slist = yypvt[-1].slist; 
} break;

case YYr131: {	/* slot_assign :  IDENT '=' expr ';' */
#line 485 "console/gram.y"
 yyval.slist = SlotAssignNode::alloc(NULL, NULL, yypvt[-3].s, yypvt[-1].expr); 
} break;

case YYr132: {	/* slot_assign :  rwDATABLOCK '=' expr ';' */
#line 487 "console/gram.y"
 yyval.slist = SlotAssignNode::alloc(NULL, NULL, StringTable->insert("datablock"), yypvt[-1].expr); 
} break;

case YYr133: {	/* slot_assign :  IDENT '[' aidx_expr ']' '=' expr ';' */
#line 489 "console/gram.y"
 yyval.slist = SlotAssignNode::alloc(NULL, yypvt[-4].expr, yypvt[-6].s, yypvt[-1].expr); 
} break;

case YYr134: {	/* aidx_expr :  expr */
#line 494 "console/gram.y"
 yyval.expr = yypvt[0].expr; 
} break;

case YYr135: {	/* aidx_expr :  aidx_expr ',' expr */
#line 496 "console/gram.y"
 yyval.expr = CommaCatExprNode::alloc(yypvt[-2].expr, yypvt[0].expr); 
} break;
#line 314 "console/yyparse.c"
	case YYrACCEPT:
		YYACCEPT;
	case YYrERROR:
		goto yyError;
	}

	/*
	 *	Look up next state in goto table.
	 */

	yyp = &yygo[yypgo[yyi]];
	yyq = yyp++;
	yyi = *yyps;
	while (yyi < *yyp++)
		;

	yystate = yyneg(yyi == *--yyp? YYQYYP: *yyq);
#if YYDEBUG
	if (CMDdebug)
		YY_TRACE(yyShowGoto)
#endif
	goto yyStack;

yyerrlabel:	;		/* come here from YYERROR	*/
/*
#pragma used yyerrlabel
 */
	yyerrflag = 1;
	if (yyi == YYrERROR) {
		yyps--;
		yypv--;
#if YYDEBUG
		yytp--;
#endif
	}

yyError:
	switch (yyerrflag) {

	case 0:		/* new error */
		yynerrs++;
		yyi = CMDchar;
		CMDerror(m_textmsg(4969, "Syntax error", "E"));
		if (yyi != CMDchar) {
			/* user has changed the current token */
			/* try again */
			yyerrflag++;	/* avoid loops */
			goto yyEncore;
		}

	case 1:		/* partially recovered */
	case 2:
		yyerrflag = 3;	/* need 3 valid shifts to recover */
			
		/*
		 *	Pop states, looking for a
		 *	shift on `error'.
		 */

		for ( ; yyps > yys; yyps--, yypv--
#if YYDEBUG
					, yytp--
#endif
		) {
#ifdef YACC_WINDOWS
			if (*yyps >= Sizeof_yypact)
#else /* YACC_WINDOWS */
			if (*yyps >= sizeof yypact/sizeof yypact[0])
#endif /* YACC_WINDOWS */
				continue;
			yyp = &yyact[yypact[*yyps]];
			yyq = yyp;
			do {
				if (YYERRCODE == *yyp) {
					yyp++;
					yystate = yyneg(YYQYYP);
					goto yyStack;
				}
			} while (*yyp++ > YYTOKEN_BASE);
		
			/* no shift in this state */
#if YYDEBUG
			if (CMDdebug && yyps > yys+1)
				YY_TRACE(yyShowErrRecovery)
#endif
			/* pop stacks; try again */
		}
		/* no shift on error - abort */
		break;

	case 3:
		/*
		 *	Erroneous token after
		 *	an error - discard it.
		 */

		if (CMDchar == 0)  /* but not EOF */
			break;
#if YYDEBUG
		if (CMDdebug)
			YY_TRACE(yyShowErrDiscard)
#endif
		yyclearin;
		goto yyEncore;	/* try again in same state */
	}
	YYABORT;

#ifdef YYALLOC
yyReturn:
	CMDlval = save_yylval;
	yyval = save_yyval;
	yypvt = save_yypvt;
	CMDchar = save_yychar;
	yyerrflag = save_yyerrflag;
	yynerrs = save_yynerrs;
	free((char *)yys);
	free((char *)yyv);
#if YYDEBUG
	free((char *)yytypev);
#endif
	return(retval);
#endif
}

		
#if YYDEBUG
/*
 * Return type of token
 */
int
yyGetType(tok)
int tok;
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->type;
	return 0;
}
/*
 * Print a token legibly.
 */
char *
yyptok(tok)
int tok;
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->name;
	return "";
}

/*
 * Read state 'num' from YYStatesFile
 */
#ifdef YYTRACE

static char *
yygetState(num)
int num;
{
	int	size;
	static FILE *yyStatesFile = (FILE *) 0;
	static char yyReadBuf[YYMAX_READ+1];

	if (yyStatesFile == (FILE *) 0
	 && (yyStatesFile = fopen(YYStatesFile, "r")) == (FILE *) 0)
		return "yyExpandName: cannot open states file";

	if (num < yynstate - 1)
		size = (int)(yyStates[num+1] - yyStates[num]);
	else {
		/* length of last item is length of file - ptr(last-1) */
		if (fseek(yyStatesFile, 0L, 2) < 0)
			goto cannot_seek;
		size = (int) (ftell(yyStatesFile) - yyStates[num]);
	}
	if (size < 0 || size > YYMAX_READ)
		return "yyExpandName: bad read size";
	if (fseek(yyStatesFile, yyStates[num], 0) < 0) {
	cannot_seek:
		return "yyExpandName: cannot seek in states file";
	}

	(void) fread(yyReadBuf, 1, size, yyStatesFile);
	yyReadBuf[size] = '\0';
	return yyReadBuf;
}
#endif /* YYTRACE */
/*
 * Expand encoded string into printable representation
 * Used to decode yyStates and yyRules strings.
 * If the expansion of 's' fits in 'buf', return 1; otherwise, 0.
 */
int
yyExpandName(num, isrule, buf, len)
int num, isrule;
char * buf;
int len;
{
	int	i, n, cnt, type;
	char	* endp, * cp;
	char	*s;

	if (isrule)
		s = yyRules[num].name;
	else
#ifdef YYTRACE
		s = yygetState(num);
#else
		s = "*no states*";
#endif

	for (endp = buf + len - 8; *s; s++) {
		if (buf >= endp) {		/* too large: return 0 */
		full:	(void) strcpy(buf, " ...\n");
			return 0;
		} else if (*s == '%') {		/* nonterminal */
			type = 0;
			cnt = yynvar;
			goto getN;
		} else if (*s == '&') {		/* terminal */
			type = 1;
			cnt = yyntoken;
		getN:
			if (cnt < 100)
				i = 2;
			else if (cnt < 1000)
				i = 3;
			else
				i = 4;
			for (n = 0; i-- > 0; )
				n = (n * 10) + *++s - '0';
			if (type == 0) {
				if (n >= yynvar)
					goto too_big;
				cp = yysvar[n];
			} else if (n >= yyntoken) {
			    too_big:
				cp = "<range err>";
			} else
				cp = yyTokenTypes[n].name;

			if ((i = strlen(cp)) + buf > endp)
				goto full;
			(void) strcpy(buf, cp);
			buf += i;
		} else
			*buf++ = *s;
	}
	*buf = '\0';
	return 1;
}
#ifndef YYTRACE
/*
 * Show current state of CMDparse
 */
void
yyShowState(tp)
yyTraceItems * tp;
{
	short * p;
	YYSTYPE * q;

	printf(
	    m_textmsg(2828, "state %d (%d), char %s (%d)\n", "I num1 num2 char num3"),
	      yysmap[tp->state], tp->state,
	      yyptok(tp->lookahead), tp->lookahead);
}
/*
 * show results of reduction
 */
void
yyShowReduce(tp)
yyTraceItems * tp;
{
	printf("reduce %d (%d), pops %d (%d)\n",
		yyrmap[tp->rule], tp->rule,
		tp->states[tp->nstates - tp->npop],
		yysmap[tp->states[tp->nstates - tp->npop]]);
}
void
yyShowRead(val)
int val;
{
	printf(m_textmsg(2829, "read %s (%d)\n", "I token num"), yyptok(val), val);
}
void
yyShowGoto(tp)
yyTraceItems * tp;
{
	printf(m_textmsg(2830, "goto %d (%d)\n", "I num1 num2"), yysmap[tp->state], tp->state);
}
void
yyShowShift(tp)
yyTraceItems * tp;
{
	printf(m_textmsg(2831, "shift %d (%d)\n", "I num1 num2"), yysmap[tp->state], tp->state);
}
void
yyShowErrRecovery(tp)
yyTraceItems * tp;
{
	short	* top = tp->states + tp->nstates - 1;

	printf(
	m_textmsg(2832, "Error recovery pops state %d (%d), uncovers %d (%d)\n", "I num1 num2 num3 num4"),
		yysmap[*top], *top, yysmap[*(top-1)], *(top-1));
}
void
yyShowErrDiscard(tp)
yyTraceItems * tp;
{
	printf(m_textmsg(2833, "Error recovery discards %s (%d), ", "I token num"),
		yyptok(tp->lookahead), tp->lookahead);
}
#endif	/* ! YYTRACE */
#endif	/* YYDEBUG */
