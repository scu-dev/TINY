/****************************************************/
/* File: scan.c                                     */
/* The scanner implementation for the TINY compiler */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "SCAN.H"
#include "GLOBALS.H"
#include "UTIL.H"

#include <ctype.h>
#include <string.h>

/* States in scanner DFA. */
typedef enum {
    START, INASSIGN, INCOMMENT, INNUM, INFLOAT, INID, INLT, INGT,
    INPP, INAND, INOR, INSTRING, DONE
} StateType;

/* Lexeme of identifier or reserved word. */
char tokenString[MAXTOKENLEN + 1];

/* `BUFLEN` = length of the input buffer for source code lines. */
#define BUFLEN 256

static char lineBuf[BUFLEN]; /* Holds the current line */
static int linepos = 0; /* Current position in LineBuf */
static int bufsize = 0; /* Current size of buffer string */
static int EOF_flag = FALSE; /* Corrects ungetNextChar behavior on EOF */

/* `getNextChar` fetches the next non-blank character from lineBuf, reading in a new line if needed. */
static int getNextChar() {
    if (!(linepos < bufsize)) {
        lineno++;
        if (fgets(lineBuf, BUFLEN - 1, source)) {
            if (EchoSource) fprintf(listing, "%4d: %s", lineno, lineBuf);
            bufsize = strlen(lineBuf);
            linepos = 0;
            return lineBuf[linepos++];
        }
        else {
            EOF_flag = TRUE;
            return EOF;
        }
    }
    else return lineBuf[linepos++];
}

/* `ungetNextChar` backtracks one character in lineBuf. */
static void ungetNextChar() {
    if (!EOF_flag) linepos--;
}

/* Lookup table of reserved words. */
static struct {
    const char* str;
    TokenType tok;
} reservedWords[MAXRESERVED] = {
    {"if", IF}, {"then", THEN}, {"else", ELSE}, {"end", END},
    {"repeat", REPEAT}, {"until", UNTIL}, {"read", READ},
    {"write", WRITE}, {"int", INT}, {"float", FLOAT}
};

/* Lookup an identifier to see if it is a reserved word. Uses linear search. */
static TokenType reservedLookup(const char* s) {
    int i;
    for (i = 0; i < MAXRESERVED; i++) {
        if (!strcmp(s, reservedWords[i].str)) return reservedWords[i].tok;
    }
    return ID;
}

/* Function `getToken` returns the next token in source file. */
TokenType getToken() {
    int tokenStringIndex = 0; /* Index for storing into tokenString */
    TokenType currentToken; /* Holds current token to be returned */
    StateType state = START; /* Current state - always begins at START */
    int save; /* Flag to indicate save to tokenString */
    while (state != DONE) {
        int c = getNextChar();
        save = TRUE;
        switch (state) {
        case START:
            if (isdigit(c)) state = INNUM;
            else if (isalpha(c)) state = INID;
            else if (c == ':') state = INASSIGN;
            else if ((c == ' ') || (c == '\t') || (c == '\n')) save = FALSE;
            else if (c == '{') {
                save = FALSE;
                state = INCOMMENT;
            }
            else if (c == '"') {
                save = FALSE;
                state = INSTRING;
            }
            else {
                state = DONE;
                switch (c) {
                case EOF:
                    save = FALSE;
                    currentToken = ENDFILE;
                    break;
                case '=':
                    currentToken = EQ;
                    break;
                case '<':
                    state = INLT;
                    break;
                case '>':
                    state = INGT;
                    break;
                case '+':
                    state = INPP;
                    break;
                case '&':
                    state = INAND;
                    break;
                case '|':
                    state = INOR;
                    break;
                case '-':
                    currentToken = MINUS;
                    break;
                case '*':
                    currentToken = TIMES;
                    break;
                case '/':
                    currentToken = OVER;
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
                case ',':
                    currentToken = COMMA;
                    break;
                default:
                    currentToken = ERROR;
                    break;
                }
            }
            break;
        case INCOMMENT:
            save = FALSE;
            if (c == EOF) {
                state = DONE;
                currentToken = ENDFILE;
            }
            else if (c == '}') state = START;
            break;
        case INASSIGN:
            state = DONE;
            if (c == '=') currentToken = ASSIGN;
            else {
                ungetNextChar();
                save = FALSE;
                currentToken = ERROR;
            }
            break;
        case INNUM:
            if (c == '.') state = INFLOAT;
            else if (!isdigit(c)) {
                ungetNextChar();
                save = FALSE;
                state = DONE;
                currentToken = NUM;
            }
            break;
        case INFLOAT:
            if (!isdigit(c)) {
                ungetNextChar();
                save = FALSE;
                state = DONE;
                currentToken = NUM;
            }
            break;
        case INID:
            if (!isalpha(c)) {
                ungetNextChar();
                save = FALSE;
                state = DONE;
                currentToken = ID;
            }
            break;
        case INLT:
            state = DONE;
            if (c == '=') currentToken = LEQ;
            else {
                ungetNextChar();
                save = FALSE;
                currentToken = LT;
            }
            break;
        case INGT:
            state = DONE;
            if (c == '=') currentToken = GEQ;
            else {
                ungetNextChar();
                save = FALSE;
                currentToken = GT;
            }
            break;
        case INPP:
            state = DONE;
            if (c == '+') currentToken = PP;
            else {
                ungetNextChar();
                save = FALSE;
                currentToken = PLUS;
            }
            break;
        case INAND:
            state = DONE;
            if (c == '&') currentToken = AND;
            else {
                ungetNextChar();
                save = FALSE;
                currentToken = ERROR;
            }
            break;
        case INOR:
            state = DONE;
            if (c == '|') currentToken = OR;
            else {
                ungetNextChar();
                save = FALSE;
                currentToken = ERROR;
            }
            break;
        case INSTRING:
            if (c == '"') {
                save = FALSE;
                state = DONE;
                currentToken = STRING;
            }
            else if ((c == EOF) || (c == '\n')) {
                save = FALSE;
                state = DONE;
                currentToken = ERROR;
            }
            break;
        case DONE:
        default:
            fprintf(listing, "Scanner Bug: state= %d\n", state);
            state = DONE;
            currentToken = ERROR;
            break;
        }
        if (save && (tokenStringIndex <= MAXTOKENLEN)) tokenString[tokenStringIndex++] = (char)c;
        if (state == DONE) {
            tokenString[tokenStringIndex] = '\0';
            if (currentToken == ID) currentToken = reservedLookup(tokenString);
        }
    }
    if (TraceScan) {
        fprintf(listing, "\t%d: ", lineno);
        printToken(currentToken, tokenString);
    }
    return currentToken;
}