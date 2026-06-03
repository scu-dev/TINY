/****************************************************/
/* File: code.c                                     */
/* TM Code emitting utilities                       */
/* implementation for the TINY compiler             */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "CODE.H"
#include "GLOBALS.H"

/* TM location number for current instruction emission */
static int emitLoc = 0;

/* Highest TM location emitted so far. For use in conjunction with emitSkip, emitBackup, and emitRestore. */
static int highEmitLoc = 0;

/* Procedure `emitComment` prints a comment line with comment `c` in the code file. */
void emitComment(const char* c) {
    if (TraceCode) fprintf(code, "* %s\n", c);
}

/* Procedure `emitRO` emits a register-only TM instruction. */
void emitRO(const char* op, int r, int s, int t, const char* c) {
    fprintf(code, "%3d:  %5s  %d,%d,%d ", emitLoc++, op, r, s, t);
    if (TraceCode) fprintf(code, "\t%s", c);
    fprintf(code, "\n");
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}

/* Procedure `emitRM` emits a register-to-memory TM instruction. */
void emitRM(const char* op, int r, int d, int s, const char* c) {
    fprintf(code, "%3d:  %5s  %d,%d(%d) ", emitLoc++, op, r, d, s);
    if (TraceCode) fprintf(code, "\t%s", c);
    fprintf(code, "\n");
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}

/* Procedure `emitRMFloat` emits a register-to-memory TM instruction whose displacement is a floating constant. */
void emitRMFloat(const char* op, int r, double d, int s, const char* c) {
    fprintf(code, "%3d:  %5s  %d,%.15g(%d) ", emitLoc++, op, r, d, s);
    if (TraceCode) fprintf(code, "\t%s", c);
    fprintf(code, "\n");
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}

/* Function `emitSkip` skips `howMany` code locations for later backpatch. */
int emitSkip(int howMany) {
    int i = emitLoc;
    emitLoc += howMany;
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
    return i;
}

/* Procedure `emitBackup` backs up to `loc`, a previously skipped location. */
void emitBackup(int loc) {
    if (loc > highEmitLoc) emitComment("BUG in emitBackup");
    emitLoc = loc;
}

/* Procedure `emitRestore` restores the current code position to the highest previously unemitted position. */
void emitRestore() {
    emitLoc = highEmitLoc;
}

/* Procedure `emitRM_Abs` converts an absolute reference to a pc-relative reference. */
void emitRM_Abs(const char* op, int r, int a, const char* c) {
    fprintf(code, "%3d:  %5s  %d,%d(%d) ", emitLoc, op, r, a - (emitLoc + 1), pc);
    ++emitLoc;
    if (TraceCode) fprintf(code, "\t%s", c);
    fprintf(code, "\n");
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}