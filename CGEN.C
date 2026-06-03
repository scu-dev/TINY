/****************************************************/
/* File: cgen.c                                     */
/* The code generator implementation                */
/* for the TINY compiler                            */
/* (generates code for the TM machine)              */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "CGEN.H"
#include "GLOBALS.H"
#include "SYMTAB.H"
#include "CODE.H"

#include <stdlib.h>
#include <string.h>

/* `tmpOffset` is the memory offset for temps. */
static int tmpOffset = 0;
static int stringCount = 0;

/* Prototype for internal recursive code generator. */
static void cGen(TreeNode* tree);

static void emitStringDefinition(int index, char* value) {
    int i;
    fprintf(code, "* STR %d \"", index);
    for (i = 0; value[i] != '\0'; i++) {
        if ((value[i] == '"') || (value[i] == '\\')) fputc('\\', code);
        fputc(value[i], code);
    }
    fprintf(code, "\"\n");
}

static int addStringLiteral(char* value) {
    int index = stringCount++;
    emitStringDefinition(index, value);
    return index;
}

static void genLogicalAnd(TreeNode* tree) {
    int leftFalseLoc, rightFalseLoc, endLoc, falseLoc, currentLoc;
    cGen(tree->child[0]);
    leftFalseLoc = emitSkip(1);
    cGen(tree->child[1]);
    rightFalseLoc = emitSkip(1);
    emitRM("LDC", ac, 1, 0, "and: true case");
    endLoc = emitSkip(1);
    falseLoc = emitSkip(0);
    emitRM("LDC", ac, 0, 0, "and: false case");
    currentLoc = emitSkip(0);
    emitBackup(leftFalseLoc);
    emitRM_Abs("JEQ", ac, falseLoc, "and: left false");
    emitBackup(rightFalseLoc);
    emitRM_Abs("JEQ", ac, falseLoc, "and: right false");
    emitBackup(endLoc);
    emitRM_Abs("LDA", pc, currentLoc, "and: jmp to end");
    emitRestore();
}

static void genLogicalOr(TreeNode* tree) {
    int leftTrueLoc, rightTrueLoc, endLoc, trueLoc, currentLoc;
    cGen(tree->child[0]);
    leftTrueLoc = emitSkip(1);
    cGen(tree->child[1]);
    rightTrueLoc = emitSkip(1);
    emitRM("LDC", ac, 0, 0, "or: false case");
    endLoc = emitSkip(1);
    trueLoc = emitSkip(0);
    emitRM("LDC", ac, 1, 0, "or: true case");
    currentLoc = emitSkip(0);
    emitBackup(leftTrueLoc);
    emitRM_Abs("JNE", ac, trueLoc, "or: left true");
    emitBackup(rightTrueLoc);
    emitRM_Abs("JNE", ac, trueLoc, "or: right true");
    emitBackup(endLoc);
    emitRM_Abs("LDA", pc, currentLoc, "or: jmp to end");
    emitRestore();
}

/* Procedure `genStmt` generates code at a statement node. */
static void genStmt(TreeNode* tree) {
    TreeNode* p1;
    TreeNode* p2;
    TreeNode* p3;
    int savedLoc1, savedLoc2, currentLoc;
    int loc;
    switch (tree->kind.stmt) {
    case IfK:
        if (TraceCode) emitComment("-> if");
        p1 = tree->child[0];
        p2 = tree->child[1];
        p3 = tree->child[2];
        cGen(p1);
        savedLoc1 = emitSkip(1);
        emitComment("if: jump to else belongs here");
        cGen(p2);
        savedLoc2 = emitSkip(1);
        emitComment("if: jump to end belongs here");
        currentLoc = emitSkip(0);
        emitBackup(savedLoc1);
        emitRM_Abs("JEQ", ac, currentLoc, "if: jmp to else");
        emitRestore();
        cGen(p3);
        currentLoc = emitSkip(0);
        emitBackup(savedLoc2);
        emitRM_Abs("LDA", pc, currentLoc, "jmp to end");
        emitRestore();
        if (TraceCode) emitComment("<- if");
        break;
    case RepeatK:
        if (TraceCode) emitComment("-> repeat");
        p1 = tree->child[0];
        p2 = tree->child[1];
        savedLoc1 = emitSkip(0);
        emitComment("repeat: jump after body comes back here");
        cGen(p1);
        cGen(p2);
        emitRM_Abs("JEQ", ac, savedLoc1, "repeat: jmp back to body");
        if (TraceCode) emitComment("<- repeat");
        break;
    case AssignK:
        if (TraceCode) emitComment("-> assign");
        cGen(tree->child[0]);
        loc = st_lookup(tree->attr.name);
        emitRM("ST", ac, loc, gp, "assign: store value");
        if (TraceCode) emitComment("<- assign");
        break;
    case ReadK:
        loc = st_lookup(tree->attr.name);
        if (st_lookup_type(tree->attr.name) == Float) emitRO("INF", ac, 0, 0, "read float value");
        else emitRO("IN", ac, 0, 0, "read integer value");
        emitRM("ST", ac, loc, gp, "read: store value");
        break;
    case WriteK:
        p1 = tree->child[0];
        if ((p1 != NULL) && (p1->nodekind == ExpK) && (p1->kind.exp == StringK)) {
            int stringIndex = addStringLiteral(p1->attr.name);
            emitRM("LDC", ac, stringIndex, 0, "load string index");
            emitRO("OUTS", ac, 0, 0, "write string");
        }
        else {
            cGen(p1);
            emitRO("OUT", ac, 0, 0, "write ac");
        }
        break;
    case IntK: case FloatK: {
        int i;
        if (TraceCode) emitComment("-> decl");
        for (i = 0; i < MAXCHILDREN; i++) {
            if (
                tree->child[i] != NULL
             && tree->child[i]->nodekind == StmtK
             && tree->child[i]->kind.stmt == AssignK
            ) cGen(tree->child[i]);
        }
        if (TraceCode) emitComment("<- decl");
        break;
    }
    default:
        break;
    }
}

/* Procedure `genExp` generates code at an expression node. */
static void genExp(TreeNode* tree) {
    int loc;
    TreeNode* p1;
    TreeNode* p2;
    switch (tree->kind.exp) {
    case ConstK:
        if (TraceCode) emitComment("-> Const");
        if (tree->type == Float) emitRMFloat("LDC", ac, tree->attr.fval, 0, "load float const");
        else emitRM("LDC", ac, tree->attr.val, 0, "load const");
        if (TraceCode) emitComment("<- Const");
        break;
    case StringK:
        break;
    case IdK:
        if (TraceCode) emitComment("-> Id");
        loc = st_lookup(tree->attr.name);
        emitRM("LD", ac, loc, gp, "load id value");
        if (TraceCode) emitComment("<- Id");
        break;
    case OpK:
        if (TraceCode) emitComment("-> Op");
        p1 = tree->child[0];
        p2 = tree->child[1];
        if (tree->attr.op == PP) {
            loc = st_lookup(p1->attr.name);
            emitRM("LD", ac, loc, gp, "post++: load old value");
            emitRM("ST", ac, tmpOffset--, mp, "post++: save old value");
            emitRM("LDC", ac1, 1, 0, "post++: load increment");
            emitRO("ADD", ac, ac, ac1, "post++: increment");
            emitRM("ST", ac, loc, gp, "post++: store new value");
            emitRM("LD", ac, ++tmpOffset, mp, "post++: restore old value");
            if (TraceCode) emitComment("<- Op");
            break;
        }
        if (tree->attr.op == AND) {
            genLogicalAnd(tree);
            if (TraceCode) emitComment("<- Op");
            break;
        }
        if (tree->attr.op == OR) {
            genLogicalOr(tree);
            if (TraceCode) emitComment("<- Op");
            break;
        }
        cGen(p1);
        emitRM("ST", ac, tmpOffset--, mp, "op: push left");
        cGen(p2);
        emitRM("LD", ac1, ++tmpOffset, mp, "op: load left");
        switch (tree->attr.op) {
        case PLUS:
            emitRO("ADD", ac, ac1, ac, "op +");
            break;
        case MINUS:
            emitRO("SUB", ac, ac1, ac, "op -");
            break;
        case TIMES:
            emitRO("MUL", ac, ac1, ac, "op *");
            break;
        case OVER:
            emitRO("DIV", ac, ac1, ac, "op /");
            break;
        case LT:
            emitRO("SUB", ac, ac1, ac, "op <");
            emitRM("JLT", ac, 2, pc, "br if true");
            emitRM("LDC", ac, 0, ac, "false case");
            emitRM("LDA", pc, 1, pc, "unconditional jmp");
            emitRM("LDC", ac, 1, ac, "true case");
            break;
        case GT:
            emitRO("SUB", ac, ac1, ac, "op >");
            emitRM("JGT", ac, 2, pc, "br if true");
            emitRM("LDC", ac, 0, ac, "false case");
            emitRM("LDA", pc, 1, pc, "unconditional jmp");
            emitRM("LDC", ac, 1, ac, "true case");
            break;
        case LEQ:
            emitRO("SUB", ac, ac1, ac, "op <=");
            emitRM("JLE", ac, 2, pc, "br if true");
            emitRM("LDC", ac, 0, ac, "false case");
            emitRM("LDA", pc, 1, pc, "unconditional jmp");
            emitRM("LDC", ac, 1, ac, "true case");
            break;
        case GEQ:
            emitRO("SUB", ac, ac1, ac, "op >=");
            emitRM("JGE", ac, 2, pc, "br if true");
            emitRM("LDC", ac, 0, ac, "false case");
            emitRM("LDA", pc, 1, pc, "unconditional jmp");
            emitRM("LDC", ac, 1, ac, "true case");
            break;
        case EQ:
            emitRO("SUB", ac, ac1, ac, "op ==");
            emitRM("JEQ", ac, 2, pc, "br if true");
            emitRM("LDC", ac, 0, ac, "false case");
            emitRM("LDA", pc, 1, pc, "unconditional jmp");
            emitRM("LDC", ac, 1, ac, "true case");
            break;
        default:
            emitComment("BUG: Unknown operator");
            break;
        }
        if (TraceCode) emitComment("<- Op");
        break;
    default:
        break;
    }
}

/* Procedure `cGen` recursively generates code by tree traversal. */
static void cGen(TreeNode* tree) {
    if (tree != NULL) {
        switch (tree->nodekind) {
        case StmtK:
            genStmt(tree);
            break;
        case ExpK:
            genExp(tree);
            break;
        default:
            break;
        }
        cGen(tree->sibling);
    }
}

/* Procedure `codeGen` generates code to a code file by traversal of the syntax tree. */
void codeGen(TreeNode* syntaxTree, char* codefile) {
    char* s = (char*)malloc(strlen(codefile) + 7);
    strcpy(s, "File: ");
    strcat(s, codefile);
    emitComment("TINY Compilation to TM Code");
    emitComment(s);
    emitComment("Standard prelude:");
    emitRM("LD", mp, 0, ac, "load maxaddress from location 0");
    emitRM("ST", ac, 0, ac, "clear location 0");
    emitComment("End of standard prelude.");
    cGen(syntaxTree);
    emitComment("End of execution.");
    emitRO("HALT", 0, 0, 0, "");
}