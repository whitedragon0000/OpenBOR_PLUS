#include "ScriptVariantInteger.h"
#include <limits.h>

/*
* Caskey, Damon V.
* 2026-06-04
*
* Set the result of a SIGNED integer math
* operation. Handles overflow and type
* promotion to 64-bit integers when necessary.
*/
HRESULT ScriptVariant_SetSignedIntegerResult(ScriptVariant *retvar, int64_t value, int force64)
{
    if (!retvar) {
        return E_FAIL;
    }

    if (!force64 && value >= (int64_t)LONG_MIN && value <= (int64_t)LONG_MAX) {
        ScriptVariant_ChangeType(retvar, VT_INTEGER);
        retvar->lVal = (LONG)value;

    } else {
        ScriptVariant_ChangeType(retvar, VT_INTEGER64);
        retvar->llVal = value;
    }

    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-06-04
*
* Set the result of an UNSIGNED integer math
* operation. Handles overflow and type
* promotion to 64-bit integers when necessary.
*/
HRESULT ScriptVariant_SetUnsignedIntegerResult(ScriptVariant *retvar, uint64_t value, int force64)
{
    if (!retvar) {
        return E_FAIL;
    }

    if (!force64 && value <= (uint64_t)LONG_MAX) {
        ScriptVariant_ChangeType(retvar, VT_INTEGER);
        retvar->lVal = (LONG)value;

    } else {
        ScriptVariant_ChangeType(retvar, VT_UINTEGER64);
        retvar->ullVal = value;
    }

    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-06-02
*
* Safely add two signed 64-bit integers, 
* checking for overflow.
*
* Returns 1 on success, 0 on overflow.
*/
int ScriptVariant_AddSigned64(int64_t left, int64_t right, int64_t *result) {

    if (!result) {
        return 0;
    }

    if ((right > 0 && left > INT64_MAX - right) ||
        (right < 0 && left < INT64_MIN - right)) {
        return 0;
    }

    *result = left + right;
    
    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-04
*
* Safely add two unsigned 64-bit integers,
* checking for overflow.
*
* Returns 1 on success, 0 on overflow.
*/
int ScriptVariant_AddUnsigned64(uint64_t left, uint64_t right, uint64_t *result) {

    if(!result) {
        return 0;
    }

    if (left > UINT64_MAX - right) {
        return 0;
    }

    *result = left + right;

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-04
*
* Safely subtract two signed 64-bit integers,
* checking for overflow.
*
* Returns 1 on success, 0 on overflow.
*/
int ScriptVariant_SubSigned64(int64_t left, int64_t right, int64_t *result) {

    if (!result) {
        return 0;
    }

    if ((right < 0 && left > INT64_MAX + right) ||
        (right > 0 && left < INT64_MIN + right)) {
        return 0;
    }

    *result = left - right;

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-04
*
* Safely subtract two unsigned 64-bit integers,
* checking for underflow.
*
* Returns 1 on success, 0 on underflow.
*/
int ScriptVariant_SubUnsigned64(uint64_t left, uint64_t right, uint64_t *result) {

    if (!result) {
        return 0;
    }

    if (left < right) {
        return 0;
    }

    *result = left - right;

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Safely multiply two signed 64-bit integers,
* checking for overflow.
*
* Returns 1 on success, 0 on overflow.
*/
int ScriptVariant_MulSigned64(int64_t left, int64_t right, int64_t *result) {
    
    if (!result) {
        return 0;
    }

    if (left > 0) {
        
        if ((right > 0 && left > INT64_MAX / right) ||
            (right < 0 && right < INT64_MIN / left)) {
            return 0;
        }
    
    } else if (left < 0) {

        if ((right > 0 && left < INT64_MIN / right) ||
            (right < 0 && left < INT64_MAX / right)) {
            return 0;
        }
    }

    *result = left * right;

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Safely multiply two unsigned 64-bit integers,
* checking for overflow.
*
* Returns 1 on success, 0 on overflow.
*/
int ScriptVariant_MulUnsigned64(uint64_t left, uint64_t right, uint64_t *result) {

    if (!result) {
        return 0;
    }

    if (right && left > UINT64_MAX / right) {
        return 0;
    }

    *result = left * right;    

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Safely divide two signed 64-bit integers.
* Fail on divide by zero and the one signed
* division overflow case.
*
* Returns 1 on success, 0 on failure.
*/
int ScriptVariant_DivSigned64(int64_t left, int64_t right, int64_t *result) {

    if (!result) {
        return 0;
    }

    /* 
    * Divide by 0 attempt.
    */

    if (right == 0) {        
        return 0;
    }

    /*
    * Overflow case: INT64_MIN / -1. This 
    * would produce a result of 2^63, which
    * cannot be represented in an int64_t.
    */

    if (left == INT64_MIN && right == -1) {
        return 0;
    }

    *result = left / right;

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Safely divide two unsigned 64-bit integers.
* Checks for divide by zero.
*
* Returns 1 on success, 0 on failure.
*/
int ScriptVariant_DivUnsigned64(uint64_t left, uint64_t right, uint64_t *result) {

    if (!result) {
        return 0;
    }

    if (right == 0) {
        return 0;
    }

    *result = left / right;

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Safely perform signed 64-bit modulo.
* Fails on modulo by zero and the one signed
* division overflow case.
*
* Returns 1 on success, 0 on failure.
*/
int ScriptVariant_ModSigned64(int64_t left, int64_t right, int64_t *result) {

    if (!result) {
        return 0;
    }

    /*
    * Modulo by 0 attempt.
    */

    if (right == 0) {
        return 0;
    }

    /*
    * Overflow case: INT64_MIN % -1. Even though
    * the mathematical remainder is 0, C evaluates
    * this through signed division rules.
    */

    if (left == INT64_MIN && right == -1) {
        return 0;
    }

    *result = left % right;

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-05
*
* Safely perform unsigned 64-bit modulo.
* Fails on modulo by zero.
*
* Returns 1 on success, 0 on failure.
*/
int ScriptVariant_ModUnsigned64(uint64_t left, uint64_t right, uint64_t *result) {

    if (!result) {
        return 0;
    }

    if (right == 0) {
        return 0;
    }

    *result = left % right;

    return 1;
}