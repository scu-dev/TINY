/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "GLOBALS.H"

/* Procedure `st_insert` inserts line numbers and memory locations into the symbol table. */
void st_insert(char* name, int lineno, int loc);

/* Procedure `st_insert_typed` inserts a declared identifier with its source type on first definition. */
void st_insert_typed(char* name, int lineno, int loc, ExpType type);

/* Function `st_lookup` returns the memory location of a variable or -1 if not found. */
int st_lookup(char* name);

/* Function `st_lookup_type` returns the declared type of a variable. */
ExpType st_lookup_type(char* name);

/* Procedure `printSymTab` prints a formatted listing of the symbol table contents to the listing file. */
void printSymTab(FILE* listing);

#endif