/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Math
// 2017-04-26
// Caskey, Damon V.
//
// Mathematical operations. Ordinal functions
// written by White Dragon.

#include "scriptcommon.h"
#include "ScriptVariantInteger.h"

#include <stdint.h>
#include <math.h>

#define MATH_PI 3.14159265358979323846264338327950288

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get a 0-359 table index from an integer
* angle variant without narrowing 64-bit
* carriers through legacy LONG.
*/
static HRESULT math_angle_index(ScriptVariant *var, int *pindex)
{
    int64_t signed_value;
    uint64_t unsigned_value;
    int index;

    if (!var || !pindex) {
        return E_FAIL;
    }

    if (var->vt == VT_UINTEGER64) {
        unsigned_value = var->ullVal;
        *pindex = (int)(unsigned_value % 360u);
        return S_OK;
    }

    if (ScriptVariant_Integer64Value(var, &signed_value) != S_OK) {
        return E_FAIL;
    }

    index = (int)(signed_value % 360);

    if (index < 0) {
        index += 360;
    }

    *pindex = index;

    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Set an integer script variant from a truncated
* decimal value. Preserves legacy VT_INTEGER
* results when possible and promotes to 64-bit
* integer carriers when needed.
*/
static HRESULT math_set_truncated_integer(ScriptVariant *pretvar, DOUBLE value)
{
    DOUBLE truncated;

    if (!pretvar || !isfinite((double)value)) {
        return E_FAIL;
    }

    truncated = trunc(value);

    if (truncated >= (DOUBLE)LONG_MIN &&
        truncated <= (DOUBLE)LONG_MAX) {

        ScriptVariant_ChangeType(pretvar, VT_INTEGER);
        pretvar->lVal = (LONG)truncated;
        return S_OK;
    }

    if (truncated >= (DOUBLE)INT64_MIN &&
        truncated < 9223372036854775808.0) {

        ScriptVariant_ChangeType(pretvar, VT_INTEGER64);
        pretvar->llVal = (int64_t)truncated;
        return S_OK;
    }

    if (truncated >= 0.0 &&
        truncated < 18446744073709551616.0) {

        ScriptVariant_ChangeType(pretvar, VT_UINTEGER64);
        pretvar->ullVal = (uint64_t)truncated;
        return S_OK;
    }

    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get sine of a degree angle as a decimal.
* Cheaper than math_ssin() for integer angles
* that can be indexed directly into the sine
* table.
*/
HRESULT math_sin(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    int index;

    if (math_angle_index(varlist[0], &index) == S_OK)
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = sin_table[index];
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get cosine of a degree angle as a decimal.
* Cheaper than math_scos() for integer angles
* that can be indexed directly into the cosine
* table.
*/
HRESULT math_cos(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    int index;

    if (math_angle_index(varlist[0], &index) == S_OK)
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = cos_table[index];
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get cosine of a degree angle as a decimal.
*/
HRESULT math_scos(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)cos(dbltemp*MATH_PI/180.0);
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get square root of a variant as a decimal.
*/
HRESULT math_sqrt(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if (ScriptVariant_DecimalValue(varlist[0], &dbltemp) == S_OK &&
        dbltemp >= 0.0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)sqrt((double)dbltemp);
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Safely raise a signed 64-bit integer base to
* a non-negative integer exponent.
*/
static int math_pow_signed64(int64_t base, uint64_t exponent, int64_t *result)
{
    int64_t current = base;
    int64_t output = 1;

    if (!result) {
        return 0;
    }

    while (exponent) {
        if (exponent & 1u) {
            if (!ScriptVariant_MulSigned64(output, current, &output)) {
                return 0;
            }
        }

        exponent >>= 1u;

        if (exponent &&
            !ScriptVariant_MulSigned64(current, current, &current)) {
            return 0;
        }
    }

    *result = output;

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Safely raise an unsigned 64-bit integer base to
* a non-negative integer exponent.
*/
static int math_pow_unsigned64(uint64_t base, uint64_t exponent, uint64_t *result)
{
    uint64_t current = base;
    uint64_t output = 1;

    if (!result) {
        return 0;
    }

    while (exponent) {
        if (exponent & 1u) {
            if (!ScriptVariant_MulUnsigned64(output, current, &output)) {
                return 0;
            }
        }

        exponent >>= 1u;

        if (exponent &&
            !ScriptVariant_MulUnsigned64(current, current, &current)) {
            return 0;
        }
    }

    *result = output;

    return 1;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* *POW!*
*
* Get punched in the face by Batman. Also
* raise one variant to the power of another.
*
* Integer operands with non-negative integer
* exponents stay in integer space when the
* result fits an integer carrier. Decimal
* operands, negative exponents, and values
* that exceed 64-bit integer range fall back
* to decimal pow() behavior.
*
* Examples:
* pow(2, 8)   -> VT_INTEGER
* pow(2, 32)  -> VT_INTEGER64 or VT_UINTEGER64 depending setter policy/LONG size
* pow(2, 63)  -> VT_UINTEGER64
* pow(-2, 63) -> VT_INTEGER64
* pow(2, -1)  -> VT_DECIMAL
* pow(2.5, 2) -> VT_DECIMAL
*/
HRESULT math_pow(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    /*
    * Decimal path preserves legacy floating-point
    * behavior when either operand is decimal.
    */
    if (varlist[0]->vt == VT_DECIMAL || varlist[1]->vt == VT_DECIMAL) {
        DOUBLE base;
        DOUBLE exponent;

        if (ScriptVariant_DecimalValue(varlist[0], &base) == S_OK &&
            ScriptVariant_DecimalValue(varlist[1], &exponent) == S_OK) {

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)pow((double)base, (double)exponent);
            return S_OK;
        }

        *pretvar = NULL;
        return E_FAIL;
    }

    /*
    * Unsigned integer power. Use this path for
    * explicit unsigned bases and non-negative
    * signed bases so results above INT64_MAX
    * can still stay integer as VT_UINTEGER64.
    */
    {
        uint64_t base;
        uint64_t exponent;
        uint64_t result;

        if (ScriptVariant_Unsigned64Value(varlist[0], &base) == S_OK &&
            ScriptVariant_Unsigned64Value(varlist[1], &exponent) == S_OK &&
            math_pow_unsigned64(base, exponent, &result)) {

            ScriptVariant_SetUnsignedIntegerResult(*pretvar, result, 0);
            return S_OK;
        }
    }

    /*
    * Signed integer power for negative signed
    * bases and signed results. The exponent must
    * be non-negative.
    */
    {
        int64_t base;
        int64_t signed_exponent;
        int64_t result;

        if (ScriptVariant_Integer64Value(varlist[0], &base) == S_OK &&
            ScriptVariant_Integer64Value(varlist[1], &signed_exponent) == S_OK &&
            signed_exponent >= 0 &&
            math_pow_signed64(base, (uint64_t)signed_exponent, &result)) {

            ScriptVariant_SetSignedIntegerResult(*pretvar, result, 0);
            return S_OK;
        }
    }

    /*
    * Fallback to legacy decimal pow() behavior.
    */
    {
        DOUBLE base;
        DOUBLE exponent;

        if (ScriptVariant_DecimalValue(varlist[0], &base) == S_OK &&
            ScriptVariant_DecimalValue(varlist[1], &exponent) == S_OK) {

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)pow((double)base, (double)exponent);
            return S_OK;
        }
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get arcsine of a variant as degrees.
* Clamps input to [-1, 1] to avoid domain errors.
*/
HRESULT math_asin(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if (ScriptVariant_DecimalValue(varlist[0], &dbltemp) == S_OK)
    {
        if (dbltemp > 1.0) {
            dbltemp = 1.0;
        }

        if (dbltemp < -1.0) {
            dbltemp = -1.0;
        }

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)(asin((double)dbltemp) * 180.0 / MATH_PI);
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get sine of a degree angle as a decimal.
*/
HRESULT math_ssin(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if (ScriptVariant_DecimalValue(varlist[0], &dbltemp) == S_OK)
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)sin(dbltemp * MATH_PI / 180.0);
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get arccosine of a variant as a decimal degree.
* Clamps input to [-1, 1] to avoid domain errors.
*/
HRESULT math_acos(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp))) {

        // Clamp for safety: acos domain is [-1, 1]
        if (dbltemp > 1.0) { dbltemp = 1.0; }
        if (dbltemp < -1.0) { dbltemp = -1.0; }

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)(acos((double)dbltemp) * 180.0 / MATH_PI);
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get arctangent of a variant as a decimal degree.
*/
HRESULT math_atan(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)(atan((double)dbltemp) * 180.0 / MATH_PI);
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get arctangent of Y/X as a decimal degree.
*/
HRESULT math_atan2(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    DOUBLE y_temp;
    DOUBLE x_temp;

    if (ScriptVariant_DecimalValue(varlist[0], &y_temp) == S_OK &&
        ScriptVariant_DecimalValue(varlist[1], &x_temp) == S_OK) {

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);

        (*pretvar)->dblVal = (DOUBLE)(atan2((double)y_temp, (double)x_temp) * 180.0 / MATH_PI);
        
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get direction angle of Y/X as a decimal degree
* normalized to [0, 360).
*/
HRESULT math_angle(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    DOUBLE y;
    DOUBLE x;
    DOUBLE degrees;

    if (ScriptVariant_DecimalValue(varlist[0], &y) == S_OK &&
        ScriptVariant_DecimalValue(varlist[1], &x) == S_OK)
    {
        degrees = (DOUBLE)(atan2((double)y, (double)x) * 180.0 / MATH_PI);

        if (degrees < 0.0) {
            degrees += 360.0;
        }

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = degrees;
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get floor value of a variant as an integer.
* Preserves integer carriers and promotes decimal
* results to 64-bit integer carriers when needed.
*/
HRESULT math_floor(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    switch (varlist[0]->vt) {
        case VT_INTEGER:
        case VT_INTEGER64:
        case VT_UINTEGER64:
            ScriptVariant_Copy(*pretvar, varlist[0]);
            return S_OK;

        default:
            break;
    }

    if (ScriptVariant_DecimalValue(varlist[0], &dbltemp) == S_OK &&
        math_set_truncated_integer(*pretvar, (DOUBLE)floor((double)dbltemp)) == S_OK) {
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get ceiling value of a variant as an integer.
* Preserves integer carriers and promotes decimal
* results to 64-bit integer carriers when needed.
*/
HRESULT math_ceil(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    switch (varlist[0]->vt) {
        case VT_INTEGER:
        case VT_INTEGER64:
        case VT_UINTEGER64:
            ScriptVariant_Copy(*pretvar, varlist[0]);
            return S_OK;

        default:
            break;
    }

    if (ScriptVariant_DecimalValue(varlist[0], &dbltemp) == S_OK &&
        math_set_truncated_integer(*pretvar, (DOUBLE)ceil((double)dbltemp)) == S_OK) {
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get truncated value of a variant as an integer.
* Preserves legacy VT_INTEGER results when possible
* and promotes to 64-bit integer carriers when needed.
*/
HRESULT math_trunc(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if (ScriptVariant_DecimalValue(varlist[0], &dbltemp) == S_OK &&
        math_set_truncated_integer(*pretvar, dbltemp) == S_OK) {
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get rounded value of a variant as a decimal.
*/
HRESULT math_round(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;

    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[0], &dbltemp)))
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)(round(dbltemp));
        return S_OK;
    }
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-06-08
*
* Get absolute value of a variant as an integer
* or decimal. Integer inputs stay in integer
* space when possible, with promotion to 64-bit
* or unsigned 64-bit carriers when needed.
*
* INT64_MIN is returned as VT_UINTEGER64 because
* its positive magnitude cannot fit signed 64-bit.
*
* abs(-5) = 5
* abs(5) = 5
*/
HRESULT math_abs(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    DOUBLE dbltemp;
    int64_t signed_value;
    uint64_t magnitude;

    (void)paramCount;

    switch (varlist[0]->vt) {
        case VT_UINTEGER64:
            ScriptVariant_Copy(*pretvar, varlist[0]);
            return S_OK;

        case VT_INTEGER:
        case VT_INTEGER64:
            if (ScriptVariant_Integer64Value(varlist[0], &signed_value) != S_OK) {
                break;
            }

            if (signed_value >= 0) {
                ScriptVariant_SetSignedIntegerResult(*pretvar, signed_value, 0);
                return S_OK;
            }

            /*
             * Avoid signed overflow for INT64_MIN.
             * abs(INT64_MIN) is 9223372036854775808,
             * which only fits unsigned 64-bit.
             */
            magnitude = (uint64_t)(-(signed_value + 1)) + UINT64_C(1);

            if (magnitude <= (uint64_t)INT64_MAX) {
                ScriptVariant_SetSignedIntegerResult(*pretvar, (int64_t)magnitude, 0);
            } else {
                ScriptVariant_SetUnsignedIntegerResult(*pretvar, magnitude, 0);
            }

            return S_OK;

        default:
            break;
    }

    if (ScriptVariant_DecimalValue(varlist[0], &dbltemp) == S_OK) {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)fabs((double)dbltemp);
        return S_OK;
    }

    *pretvar = NULL;
    return E_FAIL;
}
