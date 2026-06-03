/****************************************************/
/* File: main.c                                     */
/* Main program for TINY compiler                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdlib.h>
#include <string.h>
#include "GLOBALS.H"

/* set NO_PARSE to TRUE to get a scanner-only compiler */
#define NO_PARSE FALSE
/* set NO_ANALYZE to TRUE to get a parser-only compiler */
#define NO_ANALYZE FALSE

/* set NO_CODE to TRUE to get a compiler that does not
 * generate code
 */
#define NO_CODE FALSE

#include "UTIL.H"
#include "MAKEDOT.H"
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

/* allocate global variables */
int lineno = 0;
FILE* source;
FILE* listing;
FILE* code;

/* allocate and set tracing flags */
int EchoSource = FALSE;
int TraceScan = FALSE;
int TraceParse = FALSE;
int TraceAnalyze = FALSE;
int TraceCode = FALSE;

int Error = FALSE;

static char* fileNamePart(char* path)
{ char* slash = strrchr(path,'/');
  char* backslash = strrchr(path,'\\');
  char* name = path;
  if ((slash != NULL) && (slash + 1 > name))
    name = slash + 1;
  if ((backslash != NULL) && (backslash + 1 > name))
    name = backslash + 1;
  return name;
}

static int hasFileExtension(char* path)
{ char* name = fileNamePart(path);
  return (strchr(name,'.') != NULL);
}

static char* outputFileName(char* inputFile, const char* extension)
{ char* name = fileNamePart(inputFile);
  char* dot = strrchr(name,'.');
  int fnlen;
  char* outputFile;
  if (dot != NULL)
    fnlen = (int)(dot - inputFile);
  else
    fnlen = (int)strlen(inputFile);
  outputFile = (char*)calloc(fnlen + (int)strlen(extension) + 1,sizeof(char));
  strncpy(outputFile,inputFile,fnlen);
  strcat(outputFile,extension);
  return outputFile;
}

int main( int argc, char* argv[] )
{ TreeNode* syntaxTree;
  char pgm[120]; /* source code file name */
  char* sourceArg = NULL;
  int generateDot = FALSE;
  int argIndex;
  for (argIndex = 1; argIndex < argc; argIndex++)
  { if (strcmp(argv[argIndex],"--dot") == 0)
      generateDot = TRUE;
    else if (sourceArg == NULL)
      sourceArg = argv[argIndex];
    else
    { fprintf(stderr,"usage: %s [--dot] <filename>\n",argv[0]);
      exit(1);
    }
  }
  if (sourceArg == NULL)
  { fprintf(stderr,"usage: %s [--dot] <filename>\n",argv[0]);
    exit(1);
  }
  strcpy(pgm,sourceArg) ;
  if (!hasFileExtension(pgm))
     strcat(pgm,".tny");
  source = fopen(pgm,"r");
  if (source==NULL)
  { fprintf(stderr,"File %s not found\n",pgm);
    exit(1);
  }
  listing = stdout; /* send listing to screen */
  fprintf(listing,"\nTINY COMPILATION: %s\n",pgm);
#if NO_PARSE
  while (getToken()!=ENDFILE);
#else
  syntaxTree = parse();
  if (TraceParse) {
    fprintf(listing,"\nSyntax tree:\n");
    printTree(syntaxTree);
  }
  if (generateDot)
  { char* dotfile = outputFileName(pgm,".dot");
    outputGraphvizFormat(dotfile,syntaxTree);
    fprintf(listing,"AST written to: %s\n",dotfile);
    free(dotfile);
  }
#if !NO_ANALYZE
  if (! Error)
  { if (TraceAnalyze) fprintf(listing,"\nBuilding Symbol Table...\n");
    buildSymtab(syntaxTree);
    if (TraceAnalyze) fprintf(listing,"\nChecking Types...\n");
    typeCheck(syntaxTree);
    if (TraceAnalyze) fprintf(listing,"\nType Checking Finished\n");
  }
#if !NO_CODE
  if (! Error)
  { char* codefile;
    codefile = outputFileName(pgm,".tm");
    code = fopen(codefile,"w");
    if (code == NULL)
    { printf("Unable to open %s\n",codefile);
      exit(1);
    }
    codeGen(syntaxTree,codefile);
    fclose(code);
  }
#endif
#endif
#endif
  fclose(source);
  return 0;
}