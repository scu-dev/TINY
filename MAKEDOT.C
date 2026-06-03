#include "MAKEDOT.H"
#include "GLOBALS.H"

#include <stdlib.h>

/* Token labels used by Graphviz output. Order must match `TokenType`. */
const char* optab[32] = {
    "EOF", "ERR",
    "IF", "THEN", "ELSE", "END", "REPEAT", "UNTIL", "READ", "WRITE",
    "ID", "NUM", "STRING",
    ":=", "=", "<", "<=", ">", ">=", "+", "-", "*", "/", "(", ")", ";", ",", "++", "&&", "||",
    "INT", "FLOAT"
};

#define N 100
#define NODEPTR(t) ((void*)(t))

TreeNode* drawn_opnode[N] = {NULL};
int op_index = 0;

int isdrawn(TreeNode* t) {
    int res = FALSE;
    if (t != NULL) {
        int i;
        for (i = 0; i < N; i++) {
            if (drawn_opnode[i] == NULL) break;
            if (drawn_opnode[i] == t) {
                res = TRUE;
                break;
            }
        }
    }
    else {
        printf("treenode id null\n");
    }
    return res;
}

void CreateGraphvizFormat(FILE* pf, TreeNode* syntaxtree, unsigned depth) {
    if (syntaxtree == NULL) return;
    if (syntaxtree->nodekind == StmtK) {
        switch (syntaxtree->kind.stmt) {
        case IfK:
            fprintf(pf, "\"%p\"[label = \"[IfK]\"];\n", NODEPTR(syntaxtree));
            CreateGraphvizFormat(pf, syntaxtree->child[0], depth + 1);
            CreateGraphvizFormat(pf, syntaxtree->child[1], depth + 1);
            if (syntaxtree->child[2]) CreateGraphvizFormat(pf, syntaxtree->child[2], depth + 1);
            fprintf(pf, "\"%p\"->\"%p\"[label = \"cond\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
            fprintf(pf, "\"%p\"->\"%p\"[label = \"then\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[1]));
            if (syntaxtree->child[2]) {
                fprintf(pf, "\"%p\"->\"%p\"[label = \"else\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[2]));
            }
            break;
        case RepeatK:
            fprintf(pf, "\"%p\"[label = \"[RepeatK]\"];\n", NODEPTR(syntaxtree));
            CreateGraphvizFormat(pf, syntaxtree->child[0], depth + 1);
            CreateGraphvizFormat(pf, syntaxtree->child[1], depth + 1);
            fprintf(pf, "\"%p\"->\"%p\"[label = \"body\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
            fprintf(pf, "\"%p\"->\"%p\"[label = \"until\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[1]));
            break;
        case AssignK:
            fprintf(pf, "\"%p\"[label = \"[AssignK:%s]\"];\n", NODEPTR(syntaxtree), syntaxtree->attr.name);
            CreateGraphvizFormat(pf, syntaxtree->child[0], depth + 1);
            fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
            break;
        case ReadK:
            fprintf(pf, "\"%p\"[label = \"[ReadK:%s]\"];\n", NODEPTR(syntaxtree), syntaxtree->attr.name);
            break;
        case WriteK:
            fprintf(pf, "\"%p\"[label = \"[WriteK]\"];\n", NODEPTR(syntaxtree));
            CreateGraphvizFormat(pf, syntaxtree->child[0], depth + 1);
            fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
            break;
        case IntK: {
            int i;
            fprintf(pf, "\"%p\"[label = \"[IntK]\"];\n", NODEPTR(syntaxtree));
            for (i = 0; i < MAXCHILDREN; i++) {
                if (syntaxtree->child[i]) {
                    CreateGraphvizFormat(pf, syntaxtree->child[i], depth + 1);
                    fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[i]));
                }
                else break;
            }
            break;
        }
        case FloatK: {
            int i;
            fprintf(pf, "\"%p\"[label = \"[FloatK]\"];\n", NODEPTR(syntaxtree));
            for (i = 0; i < MAXCHILDREN; i++) {
                if (syntaxtree->child[i]) {
                    CreateGraphvizFormat(pf, syntaxtree->child[i], depth + 1);
                    fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[i]));
                }
                else break;
            }
            break;
        }
        default:
            break;
        }
        if (syntaxtree->sibling) {
            CreateGraphvizFormat(pf, syntaxtree->sibling, depth + 1);
            fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->sibling));
        }
    }
    else if (syntaxtree->nodekind == ExpK) {
        switch (syntaxtree->kind.exp) {
        case OpK:
            if (!isdrawn(syntaxtree)) {
                fprintf(pf, "\"%p\"[label = \"[OpK:%s]\"];\n", NODEPTR(syntaxtree), optab[syntaxtree->attr.op]);
                if (syntaxtree->attr.op == PP) {
                    CreateGraphvizFormat(pf, syntaxtree->child[0], depth + 1);
                    fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
                }
                else {
                    CreateGraphvizFormat(pf, syntaxtree->child[0], depth + 1);
                    CreateGraphvizFormat(pf, syntaxtree->child[1], depth + 1);
                    fprintf(pf, "\"%p\"->\"%p\"[label = \"L\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
                    fprintf(pf, "\"%p\"->\"%p\"[label = \"R\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[1]));
                }
                drawn_opnode[op_index++] = syntaxtree;
            }
            break;
        case ConstK:
            if (syntaxtree->type == Float) {
                fprintf(pf, "\"%p\"[label = \"[ConstK:%g]\"];\n", NODEPTR(syntaxtree), syntaxtree->attr.fval);
            }
            else {
                fprintf(pf, "\"%p\"[label = \"[ConstK:%d]\"];\n", NODEPTR(syntaxtree), syntaxtree->attr.val);
            }
            break;
        case StringK:
            fprintf(pf, "\"%p\"[label = \"[StringK:%s]\"];\n", NODEPTR(syntaxtree), syntaxtree->attr.name);
            break;
        case IdK:
            fprintf(pf, "\"%p\"[label = \"[IdK:%s]\"];\n", NODEPTR(syntaxtree), syntaxtree->attr.name);
            break;
        default:
            break;
        }
    }
    else {
        printf("makedot wrong\n");
    }
}

void outputGraphvizFormat(const char* outputFilePath, TreeNode* syntaxtree) {
    FILE* pf = fopen(outputFilePath, "w");
    if (pf == NULL) {
        printf("Unable to open %s\n", outputFilePath);
        exit(1);
    }
    fprintf(pf, "digraph SyntaxTree {\n");
    CreateGraphvizFormat(pf, syntaxtree, 1);
    fprintf(pf, "}");
}