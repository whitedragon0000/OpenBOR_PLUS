#ifndef SCRIPTVARIANT_INTEGER_H
#define SCRIPTVARIANT_INTEGER_H

#include "ScriptVariant.h"
#include <stdint.h>

HRESULT ScriptVariant_SetSignedIntegerResult(ScriptVariant *retvar, int64_t value, int force64);
HRESULT ScriptVariant_SetUnsignedIntegerResult(ScriptVariant *retvar, uint64_t value, int force64);

int ScriptVariant_AddSigned64(int64_t left, int64_t right, int64_t *result);
int ScriptVariant_AddUnsigned64(uint64_t left, uint64_t right, uint64_t *result);
int ScriptVariant_SubSigned64(int64_t left, int64_t right, int64_t *result);
int ScriptVariant_SubUnsigned64(uint64_t left, uint64_t right, uint64_t *result);
int ScriptVariant_MulSigned64(int64_t left, int64_t right, int64_t *result);
int ScriptVariant_MulUnsigned64(uint64_t left, uint64_t right, uint64_t *result);
int ScriptVariant_DivSigned64(int64_t left, int64_t right, int64_t *result);
int ScriptVariant_DivUnsigned64(uint64_t left, uint64_t right, uint64_t *result);
int ScriptVariant_ModSigned64(int64_t left, int64_t right, int64_t *result);
int ScriptVariant_ModUnsigned64(uint64_t left, uint64_t right, uint64_t *result);

#endif