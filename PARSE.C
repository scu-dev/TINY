/****************************************************/
/* File: parse.c                                    */
/* The parser implementation for the TINY compiler  */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdlib.h>
#include <string.h>
#include "GLOBALS.H"
#include "UTIL.H"
#include "SCAN.H"
#include "PARSE.H"

static TokenType token; /* holds current token */

/* function prototypes for recursive calls */
static TreeNode* stmt_sequence(void);
static TreeNode* statement(void);
static TreeNode* if_stmt(void);
static TreeNode* repeat_stmt(void);
static TreeNode* assign_stmt(void);
static TreeNode* read_stmt(void);
static TreeNode* write_stmt(void);
static TreeNode* int_decl(void);
static TreeNode* float_decl(void);
static TreeNode* exp(void);
static TreeNode* logical_or_exp(void);
static TreeNode* logical_and_exp(void);
static TreeNode* comparison_exp(void);
static TreeNode* simple_exp(void);
static TreeNode* term(void);
static TreeNode* factor(void);

static void syntaxError(const char* message)
{ fprintf(listing,"\n>>> ");
  fprintf(listing,"Syntax error at line %d: %s",lineno,message);
  Error = TRUE;
}

static void match(TokenType expected)
{ if (token == expected) token = getToken();
  else {
    syntaxError("unexpected token -> ");
    printToken(token,tokenString);
    fprintf(listing,"      ");
  }
}

TreeNode* stmt_sequence(void)
{ TreeNode* t = statement();
  TreeNode* p = t;
  while ((token!=ENDFILE) && (token!=END) &&
         (token!=ELSE) && (token!=UNTIL))
  { TreeNode* q;
    match(SEMI);
    q = statement();
    if (q!=NULL) {
      if (t==NULL) t = p = q;
      else /* now p cannot be NULL either */
      { p->sibling = q;
        p = q;
      }
    }
  }
  return t;
}

TreeNode* statement(void)
{ TreeNode* t = NULL;
  switch (token) {
    case IF : t = if_stmt(); break;
    case REPEAT : t = repeat_stmt(); break;
    case ID : t = assign_stmt(); break;
    case READ : t = read_stmt(); break;
    case WRITE : t = write_stmt(); break;
    case INT : t = int_decl(); break;
    case FLOAT : t = float_decl(); break;
    default : syntaxError("unexpected token -> ");
              printToken(token,tokenString);
              token = getToken();
              break;
  } /* end case */
  return t;
}

TreeNode* if_stmt(void)
{ TreeNode* t = newStmtNode(IfK);
  match(IF);
  if (t!=NULL) t->child[0] = exp();
  match(THEN);
  if (t!=NULL) t->child[1] = stmt_sequence();
  if (token==ELSE) {
    match(ELSE);
    if (t!=NULL) t->child[2] = stmt_sequence();
  }
  match(END);
  return t;
}

TreeNode* repeat_stmt(void)
{ TreeNode* t = newStmtNode(RepeatK);
  match(REPEAT);
  if (t!=NULL) t->child[0] = stmt_sequence();
  match(UNTIL);
  if (t!=NULL) t->child[1] = exp();
  return t;
}

TreeNode* assign_stmt(void)
{ TreeNode* t = newStmtNode(AssignK);
  if ((t!=NULL) && (token==ID))
    t->attr.name = copyString(tokenString);
  match(ID);
  match(ASSIGN);
  if (t!=NULL) t->child[0] = exp();
  return t;
}

TreeNode* read_stmt(void)
{ TreeNode* t = newStmtNode(ReadK);
  match(READ);
  if (token != ID)
  { syntaxError("read statement requires a variable name -> ");
    printToken(token,tokenString);
    return NULL;
  }
  if ((t!=NULL) && (token==ID))
    t->attr.name = copyString(tokenString);
  match(ID);
  return t;
}

TreeNode* write_stmt(void)
{ TreeNode* t = newStmtNode(WriteK);
  match(WRITE);
  if (t!=NULL) t->child[0] = exp();
  return t;
}

/* int_decl parses: int ID [:= exp] {, ID [:= exp]}
 * Each variable becomes a child of the IntK node.
 * Uninitialized vars become IdK children; initialized
 * vars become AssignK children (with the expr as child[0]).
 */
TreeNode* int_decl(void)
{ TreeNode* t = newStmtNode(IntK);
  int i = 0;
  match(INT);
  while (token == ID && i < MAXCHILDREN)
  { char* name = copyString(tokenString);
    match(ID);
    if (token == ASSIGN)
    { TreeNode* a = newStmtNode(AssignK);
      if (a != NULL) a->attr.name = name;
      match(ASSIGN);
      if (a != NULL) a->child[0] = exp();
      if (t != NULL) t->child[i++] = a;
    }
    else
    { TreeNode* id = newExpNode(IdK);
      if (id != NULL) id->attr.name = name;
      if (t != NULL) t->child[i++] = id;
    }
    if (token == COMMA) match(COMMA);
    else break;
  }
  return t;
}

/* float_decl parses: float ID [:= exp] {, ID [:= exp]} */
TreeNode* float_decl(void)
{ TreeNode* t = newStmtNode(FloatK);
  int i = 0;
  match(FLOAT);
  while (token == ID && i < MAXCHILDREN)
  { char* name = copyString(tokenString);
    match(ID);
    if (token == ASSIGN)
    { TreeNode* a = newStmtNode(AssignK);
      if (a != NULL) a->attr.name = name;
      match(ASSIGN);
      if (a != NULL) a->child[0] = exp();
      if (t != NULL) t->child[i++] = a;
    }
    else
    { TreeNode* id = newExpNode(IdK);
      if (id != NULL) id->attr.name = name;
      if (t != NULL) t->child[i++] = id;
    }
    if (token == COMMA) match(COMMA);
    else break;
  }
  return t;
}

TreeNode* exp(void)
{ return logical_or_exp();
}

TreeNode* logical_or_exp(void)
{ TreeNode* t = logical_and_exp();
  while (token==OR)
  { TreeNode* p = newExpNode(OpK);
    if (p!=NULL) {
      p->child[0] = t;
      p->attr.op = token;
      t = p;
      match(token);
      t->child[1] = logical_and_exp();
    }
  }
  return t;
}

TreeNode* logical_and_exp(void)
{ TreeNode* t = comparison_exp();
  while (token==AND)
  { TreeNode* p = newExpNode(OpK);
    if (p!=NULL) {
      p->child[0] = t;
      p->attr.op = token;
      t = p;
      match(token);
      t->child[1] = comparison_exp();
    }
  }
  return t;
}

TreeNode* comparison_exp(void)
{ TreeNode* t = simple_exp();
  if ((token==LT)||(token==LEQ)||(token==GT)||(token==GEQ)||(token==EQ)) {
    TreeNode* p = newExpNode(OpK);
    if (p!=NULL) {
      p->child[0] = t;
      p->attr.op = token;
      t = p;
    }
    match(token);
    if (t!=NULL)
      t->child[1] = simple_exp();
  }
  return t;
}

TreeNode* simple_exp(void)
{ TreeNode* t = term();
  while ((token==PLUS)||(token==MINUS))
  { TreeNode* p = newExpNode(OpK);
    if (p!=NULL) {
      p->child[0] = t;
      p->attr.op = token;
      t = p;
      match(token);
      t->child[1] = term();
    }
  }
  return t;
}

TreeNode* term(void)
{ TreeNode* t = factor();
  while ((token==TIMES)||(token==OVER))
  { TreeNode* p = newExpNode(OpK);
    if (p!=NULL) {
      p->child[0] = t;
      p->attr.op = token;
      t = p;
      match(token);
      p->child[1] = factor();
    }
  }
  return t;
}

TreeNode* factor(void)
{ TreeNode* t = NULL;
  switch (token) {
    case NUM :
      t = newExpNode(ConstK);
      if ((t!=NULL) && (token==NUM))
      { if (strchr(tokenString,'.') != NULL)
        { t->attr.fval = atof(tokenString);
          t->type = Float;
        }
        else
        { t->attr.val = atoi(tokenString);
          t->type = Integer;
        }
      }
      match(NUM);
      break;
    case STRING :
      t = newExpNode(StringK);
      if ((t!=NULL) && (token==STRING))
      { t->attr.name = copyString(tokenString);
        t->type = String;
      }
      match(STRING);
      break;
    case ID :
      t = newExpNode(IdK);
      if ((t!=NULL) && (token==ID))
        t->attr.name = copyString(tokenString);
      match(ID);
      if (token == PP)
      { TreeNode* p = newExpNode(OpK);
        if (p!=NULL) {
          p->child[0] = t;
          p->attr.op = PP;
          t = p;
        }
        match(PP);
      }
      break;
    case LPAREN :
      match(LPAREN);
      t = exp();
      match(RPAREN);
      break;
    case MINUS :
      /* unary minus: -NUM becomes ConstK(-n), -expr becomes OpK(MINUS,0,expr) */
      match(MINUS);
      if (token == NUM)
      { t = newExpNode(ConstK);
        if (t != NULL)
        { if (strchr(tokenString,'.') != NULL)
          { t->attr.fval = -atof(tokenString);
            t->type = Float;
          }
          else
          { t->attr.val = -atoi(tokenString);
            t->type = Integer;
          }
        }
        match(NUM);
      }
      else
      { t = newExpNode(OpK);
        if (t != NULL)
        { t->attr.op = MINUS;
          t->child[0] = newExpNode(ConstK); /* implicit 0 */
          if (t->child[0] != NULL)
          { t->child[0]->attr.val = 0;
            t->child[0]->type = Integer;
          }
          t->child[1] = factor();
        }
      }
      break;
    default:
      syntaxError("unexpected token -> ");
      printToken(token,tokenString);
      token = getToken();
      break;
    }
  return t;
}

/****************************************/
/* the primary function of the parser   */
/****************************************/
/* Function parse returns the newly 
 * constructed syntax tree
 */
TreeNode* parse(void)
{ TreeNode* t;
  token = getToken();
  t = stmt_sequence();
  if (token!=ENDFILE)
    syntaxError("Code ends before file\n");
  return t;
}