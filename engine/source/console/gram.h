#define rwDEFINE   257
#define rwENDDEF   258
#define rwDECLARE   259
#define rwBREAK   260
#define rwELSE   261
#define rwCONTINUE   262
#define rwGLOBAL   263
#define rwIF   264
#define rwNIL   265
#define rwRETURN   266
#define rwWHILE   267
#define rwENDIF   268
#define rwENDWHILE   269
#define rwENDFOR   270
#define rwDEFAULT   271
#define rwFOR   272
#define rwDATABLOCK   273
#define rwSWITCH   274
#define rwCASE   275
#define rwSWITCHSTR   276
#define rwCASEOR   277
#define rwPACKAGE   278
#define ILLEGAL_TOKEN   279
#define CHRCONST   280
#define INTCONST   281
#define TTAG   282
#define VAR   283
#define IDENT   284
#define STRATOM   285
#define TAGATOM   286
#define FLTCONST   287
#define opMINUSMINUS   288
#define opPLUSPLUS   289
#define STMT_SEP   290
#define opSHL   291
#define opSHR   292
#define opPLASN   293
#define opMIASN   294
#define opMLASN   295
#define opDVASN   296
#define opMODASN   297
#define opANDASN   298
#define opXORASN   299
#define opORASN   300
#define opSLASN   301
#define opSRASN   302
#define opCAT   303
#define opEQ   304
#define opNE   305
#define opGE   306
#define opLE   307
#define opAND   308
#define opOR   309
#define opSTREQ   310
#define opCOLONCOLON   311
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
#define opMDASN   312
#define opNDASN   313
#define opNTASN   314
#define opSTRNE   315
#define UNARY   316
extern YYSTYPE CMDlval;
