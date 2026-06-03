/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "GLOBALS.H"
#include "SYMTAB.H"
#include "ANALYZE.H"

/* counter for variable memory locations */
static int location = 0;

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode* t,
               void (*preProc) (TreeNode*),
               void (*postProc) (TreeNode*) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode* t)
{ if (t==NULL) return;
  else return;
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode( TreeNode* t)
{ switch (t->nodekind)
  { case StmtK:
      switch (t->kind.stmt)
      { case AssignK:
        case ReadK:
          if (st_lookup(t->attr.name) == -1)
          /* not yet in table, so treat as new definition */
            st_insert(t->attr.name,t->lineno,location++);
          else
          /* already in table, so ignore location,
             add line number of use only */
            st_insert(t->attr.name,t->lineno,0);
          break;
        case IntK:
        case FloatK:
          { int i;
            ExpType declaredType = (t->kind.stmt == FloatK) ? Float : Integer;
            for (i = 0; i < MAXCHILDREN; i++)
            { TreeNode* child = t->child[i];
              char* name = NULL;
              if (child == NULL) continue;
              if (child->nodekind == StmtK &&
                  child->kind.stmt == AssignK)
                name = child->attr.name;
              else if (child->nodekind == ExpK &&
                       child->kind.exp == IdK)
                name = child->attr.name;
              if (name != NULL)
              { if (st_lookup(name) == -1)
                  st_insert_typed(name,t->lineno,location++,declaredType);
                else
                  st_insert_typed(name,t->lineno,0,declaredType);
              }
            }
          }
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      { case IdK:
          if (st_lookup(t->attr.name) == -1)
          /* not yet in table, so treat as new definition */
            st_insert(t->attr.name,t->lineno,location++);
          else
          /* already in table, so ignore location, 
             add line number of use only */ 
            st_insert(t->attr.name,t->lineno,0);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode* syntaxTree)
{ traverse(syntaxTree,insertNode,nullProc);
  if (TraceAnalyze)
  { fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

static void typeError(TreeNode* t, const char* message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

static int isNumeric(ExpType type)
{ return (type == Integer) || (type == Float);
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode* t)
{ switch (t->nodekind)
  { case ExpK:
      switch (t->kind.exp)
      { case OpK:
          if (t->attr.op == PP)
          { if ((t->child[0] == NULL) ||
                (t->child[0]->nodekind != ExpK) ||
                (t->child[0]->kind.exp != IdK))
              typeError(t,"++ applied to non-variable value");
            else if (!isNumeric(t->child[0]->type))
              typeError(t,"++ applied to non-numeric value");
            if (t->child[0] != NULL)
              t->type = t->child[0]->type;
            else
              t->type = Void;
            break;
          }
          if ((t->attr.op == AND) || (t->attr.op == OR))
          { if ((t->child[0]->type != Boolean) ||
                (t->child[1]->type != Boolean))
              typeError(t,"logical operator applied to non-Boolean value");
            t->type = Boolean;
            break;
          }
          if (!isNumeric(t->child[0]->type) ||
              !isNumeric(t->child[1]->type))
            typeError(t,"Op applied to non-numeric value");
          if ((t->attr.op == EQ) || (t->attr.op == LT) ||
              (t->attr.op == LEQ) || (t->attr.op == GT) ||
              (t->attr.op == GEQ))
            t->type = Boolean;
          else if ((t->child[0]->type == Float) ||
                   (t->child[1]->type == Float))
            t->type = Float;
          else
            t->type = Integer;
          break;
        case ConstK:
          if (t->type != Float)
            t->type = Integer;
          break;
        case StringK:
          t->type = String;
          break;
        case IdK:
          t->type = st_lookup_type(t->attr.name);
          break;
        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      { case IfK:
          if (t->child[0]->type != Boolean)
            typeError(t->child[0],"if test is not Boolean");
          break;
        case AssignK:
          { ExpType leftType = st_lookup_type(t->attr.name);
            if ((leftType == Integer && t->child[0]->type != Integer) ||
                (leftType == Float && !isNumeric(t->child[0]->type)))
              typeError(t->child[0],"assignment of incompatible value");
          }
          break;
        case WriteK:
          if ((t->child[0]->type == Boolean) ||
              (t->child[0]->type == Void))
            typeError(t->child[0],"write of unsupported value");
          break;
        case RepeatK:
          if (t->child[1]->type != Boolean)
            typeError(t->child[1],"repeat test is not Boolean");
          break;
        default:
          break;
      }
      break;
    default:
      break;

  }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode* syntaxTree)
{ traverse(syntaxTree,nullProc,checkNode);
}