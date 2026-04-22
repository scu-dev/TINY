/****************************************************/
/* File: scan.c                                     */
/* The scanner implementation for the TINY compiler */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "GLOBALS.H"
#include "UTIL.H"
#include "SCAN.H"

/* states in scanner DFA */
typedef enum
   { START,INASSIGN,INBRACECOMMENT,INCOMMENT,INCOMMENTEND,
     INNUM,INFRAC,INEXPSTART,INEXPSIGN,INEXP,INLEADINGDOT,
     INID,INSLASH,INBADTOKEN,DONE }
   StateType;

/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];

/* BUFLEN = length of the input buffer for
   source code lines */
#define BUFLEN 256

static char lineBuf[BUFLEN]; /* holds the current line */
static int linepos = 0; /* current position in LineBuf */
static int bufsize = 0; /* current size of buffer string */
static int EOF_flag = FALSE; /* corrects ungetNextChar behavior on EOF */

/* getNextChar fetches the next non-blank character
   from lineBuf, reading in a new line if lineBuf is
   exhausted */
static int getNextChar(void)
{ if (!(linepos < bufsize))
  { lineno++;
    if (fgets(lineBuf,BUFLEN-1,source))
    { if (EchoSource) fprintf(listing,"%4d: %s",lineno,lineBuf);
      bufsize = strlen(lineBuf);
      linepos = 0;
      return lineBuf[linepos++];
    }
    else
    { EOF_flag = TRUE;
      return EOF;
    }
  }
  else return lineBuf[linepos++];
}

/* ungetNextChar backtracks one character
   in lineBuf */
static void ungetNextChar(void)
{ if (!EOF_flag) linepos-- ;}

static void saveChar(int c, int * tokenStringIndex)
{ if ((*tokenStringIndex < MAXTOKENLEN) && (c != EOF))
    tokenString[(*tokenStringIndex)++] = (char) c;
}

static int isTokenDelimiter(int c)
{ return (c == EOF) || (c == ' ') || (c == '\t') || (c == '\n') ||
         (c == ':') || (c == '=') || (c == '<') || (c == '>') ||
         (c == '+') || (c == '-') || (c == '*') || (c == '/') ||
         (c == '(') || (c == ')') || (c == ';') ||
         (c == '{') || (c == '}');
}

static void setTokenString(const char * s, int * tokenStringIndex)
{ strncpy(tokenString,s,MAXTOKENLEN);
  tokenString[MAXTOKENLEN] = '\0';
  *tokenStringIndex = (int) strlen(tokenString);
}

/* lookup table of reserved words */
static struct
    { char* str;
      TokenType tok;
    } reservedWords[MAXRESERVED]
   = {{"if",IF},{"then",THEN},{"else",ELSE},{"end",END},
      {"repeat",REPEAT},{"until",UNTIL},{"read",READ},
      {"write",WRITE}};

/* lookup an identifier to see if it is a reserved word */
/* uses linear search */
static TokenType reservedLookup (char * s)
{ int i;
  for (i=0;i<MAXRESERVED;i++)
    if (!strcmp(s,reservedWords[i].str))
      return reservedWords[i].tok;
  return ID;
}

/****************************************/
/* the primary function of the scanner  */
/****************************************/
/* function getToken returns the 
 * next token in source file
 */
TokenType getToken(void)
{  /* index for storing into tokenString */
   int tokenStringIndex = 0;
   /* holds current token to be returned */
   TokenType currentToken = ERROR;
   /* current state - always begins at START */
   StateType state = START;
   /* flag to indicate save to tokenString */
   int save;
   while (state != DONE)
   { int c = getNextChar();
     save = TRUE;
     switch (state)
     { case START:
         if (isdigit(c))
           state = INNUM;
         else if (c == '.')
           state = INLEADINGDOT;
         else if (isalpha(c))
           state = INID;
         else if (c == ':')
           state = INASSIGN;
         else if ((c == ' ') || (c == '\t') || (c == '\n'))
           save = FALSE;
         else if (c == '{')
         { save = FALSE;
           state = INBRACECOMMENT;
         }
         else if (c == '/')
           state = INSLASH;
         else
         { state = DONE;
           switch (c)
           { case EOF:
               save = FALSE;
               currentToken = ENDFILE;
               break;
             case '=':
               currentToken = EQ;
               break;
             case '<':
               currentToken = LT;
               break;
             case '>':
               currentToken = GT;
               break;
             case '+':
               currentToken = PLUS;
               break;
             case '-':
               currentToken = MINUS;
               break;
             case '*':
               currentToken = TIMES;
               break;
             case '(':
               currentToken = LPAREN;
               break;
             case ')':
               currentToken = RPAREN;
               break;
             case ';':
               currentToken = SEMI;
               break;
             default:
               currentToken = ERROR;
               break;
           }
         }
         break;
       case INBRACECOMMENT:
         save = FALSE;
         if (c == EOF)
         { state = DONE;
           currentToken = ERROR;
           setTokenString("unterminated comment",&tokenStringIndex);
         }
         else if (c == '}') state = START;
         break;
       case INCOMMENT:
         save = FALSE;
         if (c == EOF)
         { state = DONE;
           currentToken = ERROR;
           setTokenString("unterminated comment",&tokenStringIndex);
         }
         else if (c == '*') state = INCOMMENTEND;
         break;
       case INCOMMENTEND:
         save = FALSE;
         if (c == EOF)
         { state = DONE;
           currentToken = ERROR;
           setTokenString("unterminated comment",&tokenStringIndex);
         }
         else if (c == '/') state = START;
         else if (c != '*') state = INCOMMENT;
         break;
       case INASSIGN:
         state = DONE;
         if (c == '=')
           currentToken = ASSIGN;
         else
         { /* backup in the input */
           ungetNextChar();
           save = FALSE;
           currentToken = ERROR;
         }
         break;
       case INNUM:
         if (isdigit(c))
           ;
         else if (c == '.')
           state = INFRAC;
         else if ((c == 'e') || (c == 'E'))
           state = INEXPSTART;
         else if (isalpha(c) || (c == '_'))
         { state = INBADTOKEN;
           currentToken = ERROR;
         }
         else
         { if (c != EOF) ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = NUM;
         }
         break;
       case INFRAC:
         if (isdigit(c))
           ;
         else if ((c == 'e') || (c == 'E'))
           state = INEXPSTART;
         else if (isalpha(c) || (c == '_') || (c == '.'))
         { state = INBADTOKEN;
           currentToken = ERROR;
         }
         else
         { if (c != EOF) ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = NUM;
         }
         break;
       case INEXPSTART:
         if ((c == '+') || (c == '-'))
           state = INEXPSIGN;
         else if (isdigit(c))
           state = INEXP;
         else if (isTokenDelimiter(c))
         { if (c != EOF) ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = ERROR;
         }
         else
         { state = INBADTOKEN;
           currentToken = ERROR;
         }
         break;
       case INEXPSIGN:
         if (isdigit(c))
           state = INEXP;
         else if (isTokenDelimiter(c))
         { if (c != EOF) ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = ERROR;
         }
         else
         { state = INBADTOKEN;
           currentToken = ERROR;
         }
         break;
       case INEXP:
         if (isdigit(c))
           ;
         else if (isalpha(c) || (c == '_') || (c == '.'))
         { state = INBADTOKEN;
           currentToken = ERROR;
         }
         else
         { if (c != EOF) ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = NUM;
         }
         break;
       case INLEADINGDOT:
         if (isdigit(c))
           state = INFRAC;
         else if (isTokenDelimiter(c))
         { if (c != EOF) ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = ERROR;
         }
         else
         { state = INBADTOKEN;
           currentToken = ERROR;
         }
         break;
       case INID:
         if (!isalpha(c))
         { /* backup in the input */
           ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = ID;
         }
         break;
       case INSLASH:
         if (c == '*')
         { save = FALSE;
           tokenStringIndex = 0;
           state = INCOMMENT;
         }
         else
         { if (c != EOF) ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = OVER;
         }
         break;
       case INBADTOKEN:
         currentToken = ERROR;
         if (isTokenDelimiter(c))
         { if (c != EOF) ungetNextChar();
           save = FALSE;
           state = DONE;
         }
         break;
       case DONE:
       default: /* should never happen */
         fprintf(listing,"Scanner Bug: state= %d\n",state);
         state = DONE;
         currentToken = ERROR;
         break;
     }
     if (save) saveChar(c,&tokenStringIndex);
     if (state == DONE)
     { tokenString[tokenStringIndex] = '\0';
       if (currentToken == ID)
         currentToken = reservedLookup(tokenString);
       if (currentToken == ERROR) Error = TRUE;
     }
   }
   if (TraceScan) {
     fprintf(listing,"\t%d: ",lineno);
     printToken(currentToken,tokenString);
   }
   return currentToken;
} /* end getToken */

