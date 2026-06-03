#ifndef _MAKEDOT_H_
#define _MAKEDOT_H_

#include "GLOBALS.H"

/* Procedure `CreateGraphvizFormat` recursively writes Graphviz DOT nodes and edges for the syntax tree. */
void CreateGraphvizFormat(FILE* pf, TreeNode* syntaxtree, unsigned depth);

/* Procedure `outputGraphvizFormat` writes the syntax tree to a Graphviz DOT file. */
void outputGraphvizFormat(const char* outputFilePath, TreeNode* syntaxtree);

#endif