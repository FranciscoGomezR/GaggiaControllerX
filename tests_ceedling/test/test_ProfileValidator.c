/* =============================================================================
 * test_ProfileValidator.c — Phase 3 Ceedling port of Phase 2/3 H2 tests
 *
 * Issue: H2 — BLE input validation extracted into pure ProfileValidator module.
 *
 * This file is a direct port from tests/test_profile_validator.c with two
 * changes for Ceedling:
 *   1. No #include "fakes.h" — pure logic, zero hardware deps, no mocks needed.
 *   2. No main() — Ceedling auto-discovers and runs all void test_*() functions.
 *
 * Run (from tests_ceedling/ directory):
 *   ceedling test:test_ProfileValidator
 *   ceedling test:all
 * =============================================================================*/
#include "unity.h"
#include <math.h>
#include <string.h>

/* ProfileValidator speaks only to BLEspressoServices.h types — no SDK deps */
#include "ProfileValidator.h"

/* ---------------------------------------------------------------------------
 * Helpers: build a known-good profile
 * --------------------------------------------------------------------------- */
static bleSpressoUserdata_struct make_valid_profile(void)
{
    bleSpressoUserdata_struct p;
    memset(&p, 0, sizeof(p));
    p.temp_Target       =  93.0f;
    p.sp_BrewTemp       =  93.0f;
    p.sp_StemTemp       = 130.0f;
    p.prof_preInfusePwr =  50.0f;
    p.prof_preInfuseTmr =   3.0f;
    p.prof_InfusePwr    = 100.0f;
    p.prof_InfuseTmr    =  25.0f;
    p.Prof_DeclinePwr   =  60.0f;
    p.Prof_DeclineTmr   =  10.0f;
    p.Pid_P_term        =   9.5f;
    p.Pid_I_term        =   0.3f;
    p.Pid_Iboost_term   =   6.5f;
    p.Pid_Imax_term     = 100.0f;
    p.Pid_D_term        =   0.0f;
    p.Pid_Dlpf_term     =   0.0f;
    p.Pid_Gain_term     =   1.0f;
    p.Pid_Iwindup_term  = true;
    return p;
}

/* Ceedling: setUp/tearDown called automatically before/after each test */
void setUp(void)    {}
void tearDown(void) {}

/* ===========================================================================
 * Tests for fcn_ValidateFloat_InRange (unit-level)
 * =========================================================================== */

/* Test 1 — value in range: returns true, value unchanged */
void test_H2_SingleField_InRange_Valid(void)
{
    float v = 50.0f;
    bool ok = fcn_ValidateFloat_InRange(&v, 0.0f, 100.0f, 99.0f);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, v);
}

/* Test 2 — value above max: returns false, clamped to default */
void test_H2_SingleField_OutOfRange_Clamped(void)
{
    float v = 150.0f;
    bool ok = fcn_ValidateFloat_InRange(&v, 0.0f, 100.0f, 50.0f);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, v);
}

/* Test 3 — value at lower boundary: returns true */
void test_H2_SingleField_AtLowerBoundary_Valid(void)
{
    float v = 20.0f;
    bool ok = fcn_ValidateFloat_InRange(&v, 20.0f, 110.0f, 93.0f);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, v);
}

/* Test 4 — value at upper boundary: returns true */
void test_H2_SingleField_AtUpperBoundary_Valid(void)
{
    float v = 110.0f;
    bool ok = fcn_ValidateFloat_InRange(&v, 20.0f, 110.0f, 93.0f);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 110.0f, v);
}

/* ===========================================================================
 * Tests for fcn_ValidateAndClampProfile — full-profile validation
 * =========================================================================== */

/* Test 5 — all valid: returns PROFILE_VALID, no fields changed */
void test_H2_ValidProfile_ReturnsValid(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    profileValidation_status_t s = fcn_ValidateAndClampProfile(&p);
    TEST_ASSERT_EQUAL_INT(PROFILE_VALID, s);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  93.0f, p.temp_Target);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 130.0f, p.sp_StemTemp);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,   9.5f, p.Pid_P_term);
}

/* Test 6 — temp_Target = NaN: clamped to 93.0, returns PROFILE_CLAMPED */
void test_H2_TempTarget_NaN_ClampedToDefault(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    p.temp_Target = (float)NAN;
    profileValidation_status_t s = fcn_ValidateAndClampProfile(&p);
    TEST_ASSERT_EQUAL_INT(PROFILE_CLAMPED, s);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f, p.temp_Target);
}

/* Test 7 — temp_Target = INFINITY: clamped to 93.0, returns PROFILE_CLAMPED */
void test_H2_TempTarget_Inf_ClampedToDefault(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    p.temp_Target = (float)INFINITY;
    profileValidation_status_t s = fcn_ValidateAndClampProfile(&p);
    TEST_ASSERT_EQUAL_INT(PROFILE_CLAMPED, s);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f, p.temp_Target);
}

/* Test 8 — temp_Target = -50: clamped to 93.0 (below 20°C minimum) */
void test_H2_TempTarget_Negative_ClampedToDefault(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    p.temp_Target = -50.0f;
    profileValidation_status_t s = fcn_ValidateAndClampProfile(&p);
    TEST_ASSERT_EQUAL_INT(PROFILE_CLAMPED, s);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f, p.temp_Target);
}

/* Test 9 — temp_Target = 200: clamped to 93.0 (above 110°C maximum) */
void test_H2_TempTarget_TooHigh_ClampedToDefault(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    p.temp_Target = 200.0f;
    profileValidation_status_t s = fcn_ValidateAndClampProfile(&p);
    TEST_ASSERT_EQUAL_INT(PROFILE_CLAMPED, s);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f, p.temp_Target);
}

/* Test 10 — Pid_P_term = -1.0: clamped to 9.5 */
void test_H2_PidPterm_Negative_Clamped(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    p.Pid_P_term = -1.0f;
    fcn_ValidateAndClampProfile(&p);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.5f, p.Pid_P_term);
}

/* Test 11 — Pid_I_term = 50.0 (above 10.0 max): clamped to 0.3 */
void test_H2_PidIterm_TooHigh_Clamped(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    p.Pid_I_term = 50.0f;
    fcn_ValidateAndClampProfile(&p);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.3f, p.Pid_I_term);
}

/* Test 12 — prof_preInfuseTmr = 0.0: valid (zero is in range [0, 15]) */
void test_H2_ProfTimerZero_IsValid(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    p.prof_preInfuseTmr = 0.0f;
    profileValidation_status_t s = fcn_ValidateAndClampProfile(&p);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, p.prof_preInfuseTmr);
    (void)s;
}

/* Test 13 — Prof_DeclineTmr = -5.0: clamped to default 10.0 */
void test_H2_ProfDeclineTmr_Negative_Clamped(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    p.Prof_DeclineTmr = -5.0f;
    fcn_ValidateAndClampProfile(&p);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, p.Prof_DeclineTmr);
}

/* Test 14 — multiple fields invalid: all clamped, rest unchanged */
void test_H2_MultipleFieldsInvalid_AllClamped(void)
{
    bleSpressoUserdata_struct p = make_valid_profile();
    float saved_sp_BrewTemp = p.sp_BrewTemp;
    p.temp_Target     = (float)NAN;
    p.Pid_P_term      = -99.0f;
    p.Prof_DeclineTmr = 999.0f;

    profileValidation_status_t s = fcn_ValidateAndClampProfile(&p);

    TEST_ASSERT_EQUAL_INT(PROFILE_CLAMPED, s);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  93.0f, p.temp_Target);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,   9.5f, p.Pid_P_term);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  10.0f, p.Prof_DeclineTmr);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, saved_sp_BrewTemp, p.sp_BrewTemp);
}
