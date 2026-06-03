/****************************************************/
/* File: main.c                                     */
/* Main program for TINY compiler                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "GLOBALS.H"
#include "MAKEDOT.H"
#include "MAKETAC.H"
#include "UTIL.H"

#include <stdlib.h>
#include <string.h>

/* Set NO_PARSE to TRUE to get a scanner-only compiler. */
#define NO_PARSE FALSE
/* Set NO_ANALYZE to TRUE to get a parser-only compiler. */
#define NO_ANALYZE FALSE
/* Set NO_CODE to TRUE to get a compiler that does not generate code. */
#define NO_CODE FALSE

#if NO_PARSE
#include "SCAN.H"
#else
#include "PARSE.H"
#if !NO_ANALYZE
#include "ANALYZE.H"
#if !NO_CODE
#include "CGEN.H"
#endif
#endif
#endif

/* Allocate global variables. */
int lineno = 0;
FILE* source;
FILE* listing;
FILE* code;

/* Allocate and set tracing flags. */
int EchoSource = FALSE;
int TraceScan = FALSE;
int TraceParse = FALSE;
int TraceAnalyze = FALSE;
int TraceCode = FALSE;

int Error = FALSE;

static void printUsage(const char* programName) {
    fprintf(stderr, "usage: %s <source-file> <code-output-file> [--dot <dot-output-file>] [--tac <tac-output-file>]\n", programName);
}

int main(int argc, char* argv[]) {
    TreeNode* syntaxTree;
    char* sourceArg;
    char* codefile;
    char* dotfile = NULL;
    char* tacfile = NULL;
    int argIndex;

    if (argc < 3) {
        fprintf(stderr, "Missing source or code output file\n");
        printUsage(argv[0]);
        exit(1);
    }
    sourceArg = argv[1];
    codefile = argv[2];
    for (argIndex = 3; argIndex < argc; argIndex++) {
        if (strcmp(argv[argIndex], "--dot") == 0) {
            if (dotfile != NULL) {
                fprintf(stderr, "--dot specified more than once\n");
                printUsage(argv[0]);
                exit(1);
            }
            if (++argIndex >= argc) {
                fprintf(stderr, "Missing output file after --dot\n");
                printUsage(argv[0]);
                exit(1);
            }
            dotfile = argv[argIndex];
        }
        else if (strcmp(argv[argIndex], "--tac") == 0) {
            if (tacfile != NULL) {
                fprintf(stderr, "--tac specified more than once\n");
                printUsage(argv[0]);
                exit(1);
            }
            if (++argIndex >= argc) {
                fprintf(stderr, "Missing output file after --tac\n");
                printUsage(argv[0]);
                exit(1);
            }
            tacfile = argv[argIndex];
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[argIndex]);
            printUsage(argv[0]);
            exit(1);
        }
    }
    fopen_s(&source, sourceArg, "r");
    if (source == NULL) {
        fprintf(stderr, "File %s not found\n", sourceArg);
        exit(1);
    }
    listing = stdout; /* Send listing to screen */
    fprintf(listing, "\nTINY COMPILATION: %s\n", sourceArg);
#if NO_PARSE
    while (getToken() != ENDFILE);
#else
    syntaxTree = parse();
    if (TraceParse) {
        fprintf(listing, "\nSyntax tree:\n");
        printTree(syntaxTree);
    }
    if (dotfile != NULL) {
        outputGraphvizFormat(dotfile, syntaxTree);
        fprintf(listing, "AST written to: %s\n", dotfile);
    }
    if (tacfile != NULL) {
        outputTACFormat(tacfile, syntaxTree);
        fprintf(listing, "Three-address code written to: %s\n", tacfile);
    }
#if !NO_ANALYZE
    if (!Error) {
        if (TraceAnalyze) fprintf(listing, "\nBuilding Symbol Table...\n");
        buildSymtab(syntaxTree);
        if (TraceAnalyze) fprintf(listing, "\nChecking Types...\n");
        typeCheck(syntaxTree);
        if (TraceAnalyze) fprintf(listing, "\nType Checking Finished\n");
    }
#if !NO_CODE
    if (!Error) {
        code = fopen(codefile, "w");
        if (code == NULL) {
            printf("Unable to open %s\n", codefile);
            exit(1);
        }
        codeGen(syntaxTree, codefile);
        fclose(code);
    }
#endif
#endif
#endif
    fclose(source);
    return 0;
}