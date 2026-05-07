#include "MAKEDOT.H"
/*
用于递归输出Graphviz(DOT语言)文件
输入：输出流、语法树“根节点”、递归深度（仅用作打印记号）
输出：指定路径的Graphviz(DOT语言)文件
*/
char *optab[30] = { "EOF","ERR",
                    "IF","THEN","ELSE","END","REPEAT","UNTIL","READ","WRITE",
                    "ID","NUM","STRING",
                    ":=","=","<","<=",">",">=","+","-","*","/","(",")",";", "," ,"++",
                    "INT","FLOAT"
                  };

#define N 100
#define NODEPTR(t) ((void *)(t))
TreeNode * drawn_opnode[N] = {NULL};
int op_index = 0;
int isdrawn(TreeNode* t){/* check if op node already drawn */
    int res = FALSE;
    if (t != NULL){
        for (int i = 0; i < N; i++){
            if (drawn_opnode[i] == NULL) break;
            if (drawn_opnode[i] == t){// 指向相同的op节点
                res = TRUE;
                break;
            }
        }
    }else {
        printf("treenode id null\n");
    }
    return res;
}

void CreateGraphvizFormat(FILE* pf, TreeNode* syntaxtree, unsigned depth)
{
    if (syntaxtree == NULL) return;
    if (syntaxtree->nodekind == StmtK){
        switch (syntaxtree->kind.stmt)
        {
        case IfK:
            fprintf(pf, "\"%p\"[label = \"[IfK]\"];\n", NODEPTR(syntaxtree));
            CreateGraphvizFormat(pf, syntaxtree->child[0], depth+1);//exp
            CreateGraphvizFormat(pf, syntaxtree->child[1], depth+1);//stmt_seq
            if (syntaxtree->child[2])
                CreateGraphvizFormat(pf, syntaxtree->child[2], depth+1);//stmt_seq
            fprintf(pf, "\"%p\"->\"%p\"[label = \"cond\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
            fprintf(pf, "\"%p\"->\"%p\"[label = \"then\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[1]));
            if (syntaxtree->child[2])
                fprintf(pf, "\"%p\"->\"%p\"[label = \"else\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[2]));
            break;
        case RepeatK:
            fprintf(pf, "\"%p\"[label = \"[RepeatK]\"];\n", NODEPTR(syntaxtree));
            CreateGraphvizFormat(pf, syntaxtree->child[0], depth+1);//stmt_seq
            CreateGraphvizFormat(pf, syntaxtree->child[1], depth+1);//exp
            fprintf(pf, "\"%p\"->\"%p\"[label = \"body\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
            fprintf(pf, "\"%p\"->\"%p\"[label = \"until\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[1]));
            break;
        case AssignK:
            fprintf(pf, "\"%p\"[label = \"[AssignK:%s]\"];\n", NODEPTR(syntaxtree), syntaxtree->attr.name);
            CreateGraphvizFormat(pf, syntaxtree->child[0], depth+1);//exp
            fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
            break;
        case ReadK:
            fprintf(pf, "\"%p\"[label = \"[ReadK:%s]\"];\n", NODEPTR(syntaxtree), syntaxtree->attr.name);
            break;
        case WriteK:
            fprintf(pf, "\"%p\"[label = \"[WriteK]\"];\n", NODEPTR(syntaxtree));
            CreateGraphvizFormat(pf, syntaxtree->child[0], depth+1);//exp
            fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
            break;
        case IntK:
            fprintf(pf, "\"%p\"[label = \"[IntK]\"];\n", NODEPTR(syntaxtree));
            for (int i = 0; i < MAXCHILDREN; i++){
                if (syntaxtree->child[i]){
                    CreateGraphvizFormat(pf, syntaxtree->child[i], depth+1);//identifier or expression
                    fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[i]));
                }else break;
            }
            break;
        case FloatK:
            fprintf(pf, "\"%p\"[label = \"[FloatK]\"];\n", NODEPTR(syntaxtree));
            for (int i = 0; i < MAXCHILDREN; i++){
                if (syntaxtree->child[i]){
                    CreateGraphvizFormat(pf, syntaxtree->child[i], depth+1);//identifier or expression
                    fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[i]));
                }else break;
            }
            break;
        default:
            break;
        }
        if (syntaxtree->sibling) {
            CreateGraphvizFormat(pf, syntaxtree->sibling, depth+1);//stmt
            fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->sibling));
        }
    }
    //
    else if (syntaxtree->nodekind == ExpK){
        switch (syntaxtree->kind.exp)
        {
        case OpK:
            if (!isdrawn(syntaxtree)){// 当前op节点尚未绘制
                fprintf(pf, "\"%p\"[label = \"[OpK:%s]\"];\n", NODEPTR(syntaxtree), optab[syntaxtree->attr.op]);
                if (syntaxtree->attr.op == PP){// 自增运算只有一个孩子节点
                    CreateGraphvizFormat(pf, syntaxtree->child[0], depth+1);
                    fprintf(pf, "\"%p\"->\"%p\";\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
                }else{
                    CreateGraphvizFormat(pf, syntaxtree->child[0], depth+1);
                    CreateGraphvizFormat(pf, syntaxtree->child[1], depth+1);
                    // fprintf(pf, "\"%d\"->{\"%d\"\"%d\"};\n", syntaxtree, syntaxtree->child[0], syntaxtree->child[1]);
                    fprintf(pf, "\"%p\"->\"%p\"[label = \"L\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[0]));
                    fprintf(pf, "\"%p\"->\"%p\"[label = \"R\"];\n", NODEPTR(syntaxtree), NODEPTR(syntaxtree->child[1]));
                }
                

                drawn_opnode[op_index++] = syntaxtree;
            }
            break;
        case ConstK:
            if (syntaxtree->type == Float){
                fprintf(pf, "\"%p\"[label = \"[ConstK:%g]\"];\n", NODEPTR(syntaxtree), syntaxtree->attr.fval);
            }else{
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

/*
输出Graphviz(DOT语言)文件
输入：输出文件路径
输出：指定路径的Graphviz(DOT语言)文件
*/
void outputGraphvizFormat(const char* outputFilePath, TreeNode* syntaxtree)
{
    FILE *pf = fopen(outputFilePath, "w");
    if (pf == NULL) {
      printf("Unable to open %s\n",outputFilePath);
      exit(1);
    }
    fprintf(pf, "digraph SyntaxTree {\n");
    CreateGraphvizFormat(pf, syntaxtree, 1);
    fprintf(pf, "}");
}