/****************************************************/
/* File: parse.c                                    */
/* The parser implementation for the TINY compiler  */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "PARSE.H"
#include "GLOBALS.H"
#include "SCAN.H"
#include "UTIL.H"

#include <stdlib.h>
#include <string.h>

static TokenType token; /* Holds current token */

/* Function prototypes for recursive calls. */
static TreeNode* stmt_sequence();
static TreeNode* statement();
static TreeNode* if_stmt();
static TreeNode* repeat_stmt();
static TreeNode* assign_stmt();
static TreeNode* read_stmt();
static TreeNode* write_stmt();
static TreeNode* int_decl();
static TreeNode* float_decl();
static TreeNode* exp();
static TreeNode* logical_or_exp();
static TreeNode* logical_and_exp();
static TreeNode* comparison_exp();
static TreeNode* simple_exp();
static TreeNode* term();
static TreeNode* factor();

static void syntaxError(const char* message) {
    fprintf(listing, "\n>>> ");
    fprintf(listing, "Syntax error at line %d: %s", lineno, message);
    Error = TRUE;
}

static void match(TokenType expected) {
    if (token == expected) token = getToken();
    else {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        fprintf(listing, "      ");
    }
}

static TreeNode* stmt_sequence() {
    TreeNode* t = statement();
    TreeNode* p = t;
    while ((token != ENDFILE) && (token != END) && (token != ELSE) && (token != UNTIL)) {
        TreeNode* q;
        match(SEMI);
        q = statement();
        if (q != NULL) {
            if (t == NULL) t = p = q;
            else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

static TreeNode* statement() {
    TreeNode* t = NULL;
    switch (token) {
    case IF: t = if_stmt(); break;
    case REPEAT: t = repeat_stmt(); break;
    case ID: t = assign_stmt(); break;
    case READ: t = read_stmt(); break;
    case WRITE: t = write_stmt(); break;
    case INT: t = int_decl(); break;
    case FLOAT: t = float_decl(); break;
    default:
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
        break;
    }
    return t;
}

static TreeNode* if_stmt() {
    TreeNode* t = newStmtNode(IfK);
    match(IF);
    if (t != NULL) t->child[0] = exp();
    match(THEN);
    if (t != NULL) t->child[1] = stmt_sequence();
    if (token == ELSE) {
        match(ELSE);
        if (t != NULL) t->child[2] = stmt_sequence();
    }
    match(END);
    return t;
}

static TreeNode* repeat_stmt() {
    TreeNode* t = newStmtNode(RepeatK);
    match(REPEAT);
    if (t != NULL) t->child[0] = stmt_sequence();
    match(UNTIL);
    if (t != NULL) t->child[1] = exp();
    return t;
}

static TreeNode* assign_stmt() {
    TreeNode* t = newStmtNode(AssignK);
    if ((t != NULL) && (token == ID)) t->attr.name = copyString(tokenString);
    match(ID);
    match(ASSIGN);
    if (t != NULL) t->child[0] = exp();
    return t;
}

static TreeNode* read_stmt() {
    TreeNode* t = newStmtNode(ReadK);
    match(READ);
    if (token != ID) {
        syntaxError("read statement requires a variable name -> ");
        printToken(token, tokenString);
        return NULL;
    }
    if ((t != NULL) && (token == ID)) t->attr.name = copyString(tokenString);
    match(ID);
    return t;
}

static TreeNode* write_stmt() {
    TreeNode* t = newStmtNode(WriteK);
    match(WRITE);
    if (t != NULL) t->child[0] = exp();
    return t;
}

/* `int_decl` parses: int ID [:= exp] {, ID [:= exp]}. */
static TreeNode* int_decl() {
    TreeNode* t = newStmtNode(IntK);
    int i = 0;
    match(INT);
    while (token == ID && i < MAXCHILDREN) {
        char* name = copyString(tokenString);
        match(ID);
        if (token == ASSIGN) {
            TreeNode* a = newStmtNode(AssignK);
            if (a != NULL) a->attr.name = name;
            match(ASSIGN);
            if (a != NULL) a->child[0] = exp();
            if (t != NULL) t->child[i++] = a;
        }
        else {
            TreeNode* id = newExpNode(IdK);
            if (id != NULL) id->attr.name = name;
            if (t != NULL) t->child[i++] = id;
        }
        if (token == COMMA) match(COMMA);
        else break;
    }
    return t;
}

/* `float_decl` parses: float ID [:= exp] {, ID [:= exp]}. */
static TreeNode* float_decl() {
    TreeNode* t = newStmtNode(FloatK);
    int i = 0;
    match(FLOAT);
    while (token == ID && i < MAXCHILDREN) {
        char* name = copyString(tokenString);
        match(ID);
        if (token == ASSIGN) {
            TreeNode* a = newStmtNode(AssignK);
            if (a != NULL) a->attr.name = name;
            match(ASSIGN);
            if (a != NULL) a->child[0] = exp();
            if (t != NULL) t->child[i++] = a;
        }
        else {
            TreeNode* id = newExpNode(IdK);
            if (id != NULL) id->attr.name = name;
            if (t != NULL) t->child[i++] = id;
        }
        if (token == COMMA) match(COMMA);
        else break;
    }
    return t;
}

static TreeNode* exp() {
    return logical_or_exp();
}

static TreeNode* logical_or_exp() {
    TreeNode* t = logical_and_exp();
    while (token == OR) {
        TreeNode* p = newExpNode(OpK);
        if (p != NULL) {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            t->child[1] = logical_and_exp();
        }
    }
    return t;
}

static TreeNode* logical_and_exp() {
    TreeNode* t = comparison_exp();
    while (token == AND) {
        TreeNode* p = newExpNode(OpK);
        if (p != NULL) {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            t->child[1] = comparison_exp();
        }
    }
    return t;
}

static TreeNode* comparison_exp() {
    TreeNode* t = simple_exp();
    if ((token == LT) || (token == LEQ) || (token == GT) || (token == GEQ) || (token == EQ)) {
        TreeNode* p = newExpNode(OpK);
        if (p != NULL) {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
        }
        match(token);
        if (t != NULL) t->child[1] = simple_exp();
    }
    return t;
}

static TreeNode* simple_exp() {
    TreeNode* t = term();
    while ((token == PLUS) || (token == MINUS)) {
        TreeNode* p = newExpNode(OpK);
        if (p != NULL) {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            t->child[1] = term();
        }
    }
    return t;
}

static TreeNode* term() {
    TreeNode* t = factor();
    while ((token == TIMES) || (token == OVER)) {
        TreeNode* p = newExpNode(OpK);
        if (p != NULL) {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            p->child[1] = factor();
        }
    }
    return t;
}

static TreeNode* factor() {
    TreeNode* t = NULL;
    switch (token) {
    case NUM:
        t = newExpNode(ConstK);
        if ((t != NULL) && (token == NUM)) {
            if (strchr(tokenString, '.') != NULL) {
                t->attr.fval = atof(tokenString);
                t->type = Float;
            }
            else {
                t->attr.val = atoi(tokenString);
                t->type = Integer;
            }
        }
        match(NUM);
        break;
    case STRING:
        t = newExpNode(StringK);
        if ((t != NULL) && (token == STRING)) {
            t->attr.name = copyString(tokenString);
            t->type = String;
        }
        match(STRING);
        break;
    case ID:
        t = newExpNode(IdK);
        if ((t != NULL) && (token == ID)) t->attr.name = copyString(tokenString);
        match(ID);
        if (token == PP) {
            TreeNode* p = newExpNode(OpK);
            if (p != NULL) {
                p->child[0] = t;
                p->attr.op = PP;
                t = p;
            }
            match(PP);
        }
        break;
    case LPAREN:
        match(LPAREN);
        t = exp();
        match(RPAREN);
        break;
    case MINUS:
        /* Unary minus: -NUM becomes ConstK(-n), -expr becomes OpK(MINUS, 0, expr). */
        match(MINUS);
        if (token == NUM) {
            t = newExpNode(ConstK);
            if (t != NULL) {
                if (strchr(tokenString, '.') != NULL) {
                    t->attr.fval = -atof(tokenString);
                    t->type = Float;
                }
                else {
                    t->attr.val = -atoi(tokenString);
                    t->type = Integer;
                }
            }
            match(NUM);
        }
        else {
            t = newExpNode(OpK);
            if (t != NULL) {
                t->attr.op = MINUS;
                t->child[0] = newExpNode(ConstK); /* Implicit 0 */
                if (t->child[0] != NULL) {
                    t->child[0]->attr.val = 0;
                    t->child[0]->type = Integer;
                }
                t->child[1] = factor();
            }
        }
        break;
    default:
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
        break;
    }
    return t;
}

/* Function `parse` returns the newly constructed syntax tree. */
TreeNode* parse() {
    TreeNode* t;
    token = getToken();
    t = stmt_sequence();
    if (token != ENDFILE) syntaxError("Code ends before file\n");
    return t;
}