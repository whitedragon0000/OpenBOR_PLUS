/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef IMPORTCACHE_H
#define IMPORTCACHE_H

struct SImportNode
{
    Interpreter interpreter;
    List functions; // values are Instruction**; names are function names
};
typedef struct SImportNode ImportNode;

void ImportCache_Init(List *builtinFunctions);
ImportNode *ImportCache_ImportFile(const char *path);
void ImportCache_Clear();
void ImportNode_Clear(ImportNode *self);
void ImportNode_Free(ImportNode *self);
Instruction **ImportList_GetFunctionPointer(List *list, const char *name);

#endif

