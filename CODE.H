/****************************************************/
/* File: code.h                                     */
/* Code emitting utilities for the TINY compiler    */
/* and interface to the TM machine                  */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _CODE_H_
#define _CODE_H_

/* `pc` = program counter */
#define pc 7

/* `mp` = "memory pointer" points to top of memory (for temp storage). */
#define mp 6

/* `gp` = "global pointer" points to bottom of memory for global variable storage. */
#define gp 5

/* Accumulator */
#define ac 0

/* 2nd accumulator */
#define ac1 1

/* Procedure `emitComment` prints a comment line with comment `c` in the code file. */
void emitComment(const char* c);

/* Procedure `emitRO` emits a register-only TM instruction. */
void emitRO(const char* op, int r, int s, int t, const char* c);

/* Procedure `emitRM` emits a register-to-memory TM instruction. */
void emitRM(const char* op, int r, int d, int s, const char* c);

/* Procedure `emitRMFloat` emits a register-to-memory TM instruction whose displacement is a floating constant. */
void emitRMFloat(const char* op, int r, double d, int s, const char* c);

/* Function `emitSkip` skips `howMany` code locations for later backpatch. */
int emitSkip(int howMany);

/* Procedure `emitBackup` backs up to `loc`, a previously skipped location. */
void emitBackup(int loc);

/* Procedure `emitRestore` restores the current code position to the highest previously unemitted position. */
void emitRestore();

/* Procedure `emitRM_Abs` converts an absolute reference to a pc-relative reference. */
void emitRM_Abs(const char* op, int r, int a, const char* c);

#endif