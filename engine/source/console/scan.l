%{
#define YYLMAX 4096

#include "platform.h"
#include "console.h"
#include "ast.h"
#include "gram.h"
#include "stringTable.h"

static int Sc_ScanString(int ret);
static int Sc_ScanNum();
static int Sc_ScanVar();
static int Sc_ScanHex();

#define FLEX_DEBUG 1

//#undef input
//#undef unput
#undef CMDgetc
int CMDgetc();
static int lineIndex;
extern DataChunker consoleAllocator;

// Prototypes
void SetScanBuffer(const char *sb, const char *fn);
const char * CMDgetFileLine(int &lineNumber);
void CMDerror(char * s, ...);

%}

DIGIT	[0-9]
INTEGER	{DIGIT}+
FLOAT	({INTEGER}\.{INTEGER})|({INTEGER}(\.{INTEGER})?[eE][+-]?{INTEGER})
LETTER	[A-Za-z_]
FILECHAR [A-Za-z_\.]
VARMID	[:A-Za-z0-9_]
IDTAIL	[A-Za-z0-9_]
VARTAIL  {VARMID}*{IDTAIL}
VAR	[$%]{LETTER}{VARTAIL}*
ID	{LETTER}{IDTAIL}*
ILID [$%]{DIGIT}+{LETTER}{VARTAIL}*
FILENAME {FILECHAR}+
SPACE	[ \t\v\f]
HEXDIGIT [a-fA-F0-9]

%%
			;
{SPACE}+	{ }
"//"[^\n\r]*	;
[\r]        ;
[\n]		{lineIndex++;}
\"(\\.|[^\\"\n\r])*\"		{ return(Sc_ScanString(STRATOM)); }
\'(\\.|[^\\'\n\r])*\'		{ return(Sc_ScanString(TAGATOM)); }
"=="        return(CMDlval.i = opEQ);
"!="        return(CMDlval.i = opNE);
">="        return(CMDlval.i = opGE);
"<="        return(CMDlval.i = opLE);
"&&"        return(CMDlval.i = opAND);
"||"        return(CMDlval.i = opOR);
"::"        return(CMDlval.i = opCOLONCOLON);
"--"        return(CMDlval.i = opMINUSMINUS);
"++"        return(CMDlval.i = opPLUSPLUS);
"$="        return(CMDlval.i = opSTREQ);
"!$="       return(CMDlval.i = opSTRNE);
"<<"        return(CMDlval.i = opSHL);
">>"        return(CMDlval.i = opSHR);
"+="        return(CMDlval.i = opPLASN);
"-="        return(CMDlval.i = opMIASN);
"*="        return(CMDlval.i = opMLASN);
"/="        return(CMDlval.i = opDVASN);
"%="        return(CMDlval.i = opMODASN);
"&="        return(CMDlval.i = opANDASN);
"^="        return(CMDlval.i = opXORASN);
"|="        return(CMDlval.i = opORASN);
"<<="       return(CMDlval.i = opSLASN);
">>="       return(CMDlval.i = opSRASN);
"NL"        {CMDlval.i = '\n'; return '@'; }
"TAB"       {CMDlval.i = '\t'; return '@'; }
"SPC"       {CMDlval.i = ' '; return '@'; }
"@"         {CMDlval.i = 0; return '@'; }
"?" |
"[" |
"]" |
"(" | 
")" | 
"+" | 
"-" | 
"*" | 
"/" |
"<" | 
">" | 
"|" | 
"." | 
"!" |
":" | 
";" |
"{" | 
"}" | 
"," |
"&" |
"%" |
"^" |
"~" |
"=" {       return(CMDlval.i = CMDtext[0]); }
"or"        { CMDlval.i = lineIndex; return(rwCASEOR); }
"break"     { CMDlval.i = lineIndex; return(rwBREAK); }
"return"    { CMDlval.i = lineIndex; return(rwRETURN); }
"else"      { CMDlval.i = lineIndex; return(rwELSE); }
"while"     { CMDlval.i = lineIndex; return(rwWHILE); }
"if"        { CMDlval.i = lineIndex; return(rwIF); }
"for"       { CMDlval.i = lineIndex; return(rwFOR); }
"continue"  { CMDlval.i = lineIndex; return(rwCONTINUE); }
"function"  { CMDlval.i = lineIndex; return(rwDEFINE); }
"new"       { CMDlval.i = lineIndex; return(rwDECLARE); }
"datablock" { CMDlval.i = lineIndex; return(rwDATABLOCK); }
"case"      { CMDlval.i = lineIndex; return(rwCASE); }
"switch$"   { CMDlval.i = lineIndex; return(rwSWITCHSTR); }
"switch"    { CMDlval.i = lineIndex; return(rwSWITCH); }
"default"   { CMDlval.i = lineIndex; return(rwDEFAULT); }
"package"   { CMDlval.i = lineIndex; return(rwPACKAGE); }
"true"      { CMDlval.i = 1; return INTCONST; }
"false"      { CMDlval.i = 0; return INTCONST; }
{VAR}       return(Sc_ScanVar());
{ID}        { CMDtext[CMDleng] = 0; CMDlval.s = StringTable->insert(CMDtext); return(IDENT); }
0[xX]{HEXDIGIT}+ return(Sc_ScanHex());
{INTEGER}   { CMDtext[CMDleng] = 0; CMDlval.i = atoi(CMDtext); return INTCONST; }
{FLOAT}     return Sc_ScanNum();
{ILID}      return(ILLEGAL_TOKEN);
.           return(ILLEGAL_TOKEN);
%%

/*
 * Scan character constant.
 */

/*
 * Scan identifier.
 */

static const char *scanBuffer;
static const char *fileName;
static int scanIndex;
 
const char * CMDGetCurrentFile()
{
   return fileName;
}

int CMDGetCurrentLine()
{
   return lineIndex;
}

extern bool gConsoleSyntaxError;

void CMDerror(char *, ...)
{
   gConsoleSyntaxError = true;
   if(fileName)
      Con::errorf(ConsoleLogEntry::Script, "%s Line: %d - Syntax error.",
         fileName, lineIndex);
   else
      Con::errorf(ConsoleLogEntry::Script, "Syntax error in input.");
}

void SetScanBuffer(const char *sb, const char *fn)
{
   scanBuffer = sb;
   fileName = fn;
   scanIndex = 0;
   lineIndex = 1;
}

int CMDgetc()
{
   int ret = scanBuffer[scanIndex];
   if(ret)
      scanIndex++;
   else
      ret = -1;
   return ret;
}

int CMDwrap()
{
   return 1;
}

static int Sc_ScanVar()
{
   CMDtext[CMDleng] = 0;
	CMDlval.s = StringTable->insert(CMDtext);
	return(VAR);
}
/*
 * Scan string constant.
 */

static int charConv(int in)
{
   switch(in)
   {
      case 'r':
         return '\r';
      case 'n':
         return '\n';
      case 't':
         return '\t';
      default:
         return in;
   }
}

static int getHexDigit(char c)
{
   if(c >= '0' && c <= '9')
      return c - '0';
   if(c >= 'A' && c <= 'F')
      return c - 'A' + 10;
   if(c >= 'a' && c <= 'f')
      return c - 'a' + 10;
   return -1;
}

static int Sc_ScanString(int ret)
{
	CMDtext[CMDleng - 1] = 0;
   if(!collapseEscape(CMDtext+1))
      return -1;
	CMDlval.str = (char *) consoleAllocator.alloc(dStrlen(CMDtext));
   dStrcpy(CMDlval.str, CMDtext + 1);
	return(ret);
}

void expandEscape(char *dest, const char *src)
{
   unsigned char c;
   while((c = (unsigned char) *src++) != 0)
   {
      if(c == '\"')
      {
         *dest++ = '\\';
         *dest++ = '\"';
      }
      else if(c == '\\')
      {
         *dest++ = '\\';
         *dest++ = '\\';
      }
      else if(c == '\r')
      {
         *dest++ = '\\';
         *dest++ = 'r';
      }
      else if(c == '\n')
      {
         *dest++ = '\\';
         *dest++ = 'n';
      }
      else if(c == '\t')
      {
         *dest++ = '\\';
         *dest++ = 't';
      }
      else if(c == '\'')
      {
         *dest++ = '\\';
         *dest++ = '\'';
      }
      else if((c >= 1 && c <= 7) ||
              (c >= 11 && c <= 12) ||
              (c >= 14 && c <= 15))
      {
        /*  Remap around: \b = 0x8, \t = 0x9, \n = 0xa, \r = 0xd */
        static char expandRemap[15] = { 0x0,
                                        0x0,
                                        0x1,
                                        0x2,
                                        0x3,
                                        0x4,
                                        0x5,
                                        0x6,
                                        0x0,
                                        0x0,
                                        0x0,
                                        0x7,
                                        0x8,
                                        0x0,
                                        0x9 };

         *dest++ = '\\';
         *dest++ = 'c';
         if(c == 15)
            *dest++ = 'r';
         else if(c == 16)
            *dest++ = 'p';
         else if(c == 17)
            *dest++ = 'o';
         else 
            *dest++ = expandRemap[c] + '0';
      }
      else if(c < 32)
      {
         *dest++ = '\\';
         *dest++ = 'x';
         S32 dig1 = c >> 4;
         S32 dig2 = c & 0xf;
         if(dig1 < 10)
            dig1 += '0';
         else
            dig1 += 'A' - 10;
         if(dig2 < 10)
            dig2 += '0';
         else
            dig2 += 'A' - 10;
         *dest++ = dig1;
         *dest++ = dig2;
      }
      else
         *dest++ = c;
   }
   *dest = '\0';
}   

bool collapseEscape(char *buf)
{
   int len = dStrlen(buf) + 1;
   for(int i = 0; i < len;)
   {
      if(buf[i] == '\\')
      {
         if(buf[i+1] == 'x')
         {
            int dig1 = getHexDigit(buf[i+2]);
            if(dig1 == -1)
               return false;

            int dig2 = getHexDigit(buf[i+3]);
            if(dig2 == -1)
               return false;
            buf[i] = dig1 * 16 + dig2;
            dMemmove(buf + i + 1, buf + i + 4, len - i - 3);
            len -= 3;
            i++;
         }
         else if(buf[i+1] == 'c')
         {
            /*  Remap around: \b = 0x8, \t = 0x9, \n = 0xa, \r = 0xd */
            static char collapseRemap[10] = { 0x1,
                                              0x2,
                                              0x3,
                                              0x4,
                                              0x5,
                                              0x6,
                                              0x7,
                                              0xb,
                                              0xc,
                                              0xe };
                                            
            if(buf[i+2] == 'r')
                buf[i] = 15;
            else if(buf[i+2] == 'p')
               buf[i] = 16;
            else if(buf[i+2] == 'o')
               buf[i] = 17;
            else
            {
                int dig1 = buf[i+2] - '0';
                if(dig1 < 0 || dig1 > 9)
                   return false;
                buf[i] = collapseRemap[dig1];
            }
            // Make sure we don't put 0x1 at the beginning of the string.
            if ((buf[i] == 0x1) && (i == 0))
            {
               buf[i] = 0x2;
               buf[i+1] = 0x1;
               dMemmove(buf + i + 2, buf + i + 3, len - i - 1);
               len -= 1;
            }
            else
            {
               dMemmove(buf + i + 1, buf + i + 3, len - i - 2);
               len -= 2;
            }
            i++;         
         }
         else
         {
            buf[i] = charConv(buf[i+1]);
            dMemmove(buf + i + 1, buf + i + 2, len - i - 1);
            len--;
            i++;
         }
      }
      else
         i++;
   }
   return true;
}

static int Sc_ScanNum()
{
   CMDtext[CMDleng] = 0;
	CMDlval.f = atof(CMDtext);
	return(FLTCONST);
}

static int Sc_ScanHex()
{
   int val = 0;
   dSscanf(CMDtext, "%x", &val);
   CMDlval.i = val;
   return INTCONST;
}

