/* =============================================================================
 * test_profile_validator.c — Phase 2/3 TDD tests for ProfileValidator.c
 *
 * Issues covered:
 *   H2 (Green): BLE input validation extracted into pure ProfileValidator module.
 *
 * Strategy:
 *   ProfileValidator.c is pure logic — no hardware deps, no SDK headers.
 *   Tests call fcn_ValidateAndClampProfile() and fcn_ValidateFloat_InRange()
 *   directly with known inputs and assert return values and mutated fields.
 *   No FFF fakes needed.
 *
 * Compile (from project root):
 *   gcc <CFLAGS2> tests/test_profile_validator.c
 *       ble_espresso_app/components/Application/ProfileValidator.c
 *       tests/unity/unity.c -lm -o test_profile_validator.exe
 * =============================================================================*/
#include "unity.h"
#include <math.h>
#include <string.h>

/* ProfileValidator speaks only to BLEspressoServices.h types — no SDK deps */
#include "ProfileValidator.h"

/* ---------------------------------------------------------------------------
 * Helpers: build a known-good profile and a zeroed profile
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
    /* 0.0 is a valid pre-infuse time — no clamping for this field alone */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, p.prof_preInfuseTmr);
    /* Return value may be VALID (if only this field was checked) */
    (void)s; /* other fields may still be valid */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, p.prof_preInfuseTmr);
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
    float saved_sp_BrewTemp = p.sp_BrewTemp;  /* should not change */
    p.temp_Target   = (float)NAN;
    p.Pid_P_term    = -99.0f;
    p.Prof_DeclineTmr = 999.0f;

    profileValidation_status_t s = fcn_ValidateAndClampProfile(&p);

    TEST_ASSERT_EQUAL_INT(PROFILE_CLAMPED, s);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  93.0f, p.temp_Target);   /* clamped */
    TEST_ASSERT_FLOAT_WITHIN(0.001f,   9.5f, p.Pid_P_term);    /* clamped */
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  10.0f, p.Prof_DeclineTmr); /* clamped */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, saved_sp_BrewTemp, p.sp_BrewTemp); /* unchanged */
}

/* ---------------------------------------------------------------------------
 * Unity test runner
 * --------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_H2_SingleField_InRange_Valid);
    RUN_TEST(test_H2_SingleField_OutOfRange_Clamped);
    RUN_TEST(test_H2_SingleField_AtLowerBoundary_Valid);
    RUN_TEST(test_H2_SingleField_AtUpperBoundary_Valid);
    RUN_TEST(test_H2_ValidProfile_ReturnsValid);
    RUN_TEST(test_H2_TempTarget_NaN_ClampedToDefault);
    RUN_TEST(test_H2_TempTarget_Inf_ClampedToDefault);
    RUN_TEST(test_H2_TempTarget_Negative_ClampedToDefault);
    RUN_TEST(test_H2_TempTarget_TooHigh_ClampedToDefault);
    RUN_TEST(test_H2_PidPterm_Negative_Clamped);
    RUN_TEST(test_H2_PidIterm_TooHigh_Clamped);
    RUN_TEST(test_H2_ProfTimerZero_IsValid);
    RUN_TEST(test_H2_ProfDeclineTmr_Negative_Clamped);
    RUN_TEST(test_H2_MultipleFieldsInvalid_AllClamped);
    return UNITY_END();
}
