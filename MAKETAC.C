#include "MAKETAC.H"
#include "GLOBALS.H"

#include <stdlib.h>

typedef struct TAC {
    char dest[32];
} TAC;

static int next_temp_id = 0;

static TAC makeTAC(const char* input) {
    TAC result;
    snprintf(result.dest, sizeof(result.dest), "%s", input);
    return result;
}

static TAC emitTAC(FILE* pf, char op, const TAC* lhs, const TAC* rhs) {
    TAC result;
    snprintf(result.dest, sizeof(result.dest), "t%d", next_temp_id++);
    fprintf(pf, "%s = %s %c %s\n", result.dest, lhs->dest, op, rhs->dest);
    return result;
}

static TAC printTAC_R(FILE* pf, TreeNode* node) {
    if (node == NULL) return makeTAC("0");
    switch (node->nodekind) {
    case StmtK:
        switch (node->kind.stmt) {
        case AssignK: {
            TAC rhs = printTAC_R(pf, node->child[0]);
            fprintf(pf, "%s = %s\n", node->attr.name, rhs.dest);
            break;
        }
        case ReadK:
            fprintf(pf, "read %s\n", node->attr.name);
            break;
        case WriteK: {
            TAC value = printTAC_R(pf, node->child[0]);
            fprintf(pf, "write %s\n", value.dest);
            break;
        }
        case IntK:
        case FloatK:
        case IfK:
        case RepeatK:
            for (int i = 0; i < MAXCHILDREN; i++) if (node->child[i]) printTAC_R(pf, node->child[i]);
            break;
        default:
            fprintf(pf, "Unsupported statement type for TAC generation\n");
            break;
        }
        printTAC_R(pf, node->sibling);
        break;
    case ExpK:
        switch (node->kind.exp) {
        case OpK: {
            TAC lhs = printTAC_R(pf, node->child[0]);
            TAC rhs = printTAC_R(pf, node->child[1]);
            switch (node->attr.op) {
            case PLUS:
                return emitTAC(pf, '+', &lhs, &rhs);
            case MINUS:
                return emitTAC(pf, '-', &lhs, &rhs);
            case TIMES:
                return emitTAC(pf, '*', &lhs, &rhs);
            case OVER:
                return emitTAC(pf, '/', &lhs, &rhs);
            default:
                fprintf(pf, "Unsupported operator for TAC generation\n");
                break;
            }
        }
        case ConstK:
            if (node->type == Integer) {
                char temp[32];
                snprintf(temp, sizeof(temp), "%d", node->attr.val);
                return makeTAC(temp);
            }
            else if (node->type == Float) {
                char temp[32];
                snprintf(temp, sizeof(temp), "%f", node->attr.fval);
                return makeTAC(temp);
            }
            break;
        case IdK:
            return makeTAC(node->attr.name);
            break;
        default:
            fprintf(pf, "Unsupported node type for TAC generation\n");
            break;
        }
        break;
    }
    return makeTAC("0");
}

void outputTACFormat(const char* outputFilePath, TreeNode* syntaxtree) {
    FILE* pf = fopen(outputFilePath, "w");
    if (pf == NULL) {
        printf("Unable to open %s\n", outputFilePath);
        exit(1);
    }
    if (syntaxtree == NULL) return;
    printTAC_R(pf, syntaxtree);
    fclose(pf);
}