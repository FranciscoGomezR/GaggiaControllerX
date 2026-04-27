/* =============================================================================
 * test_x04_Numbers.c — Phase 3 Ceedling port of Phase 1 math utility tests
 *
 * Tests fcn_Constrain_WithinFloats, fcn_ChrArrayToFloat, fcn_FloatToChrArray,
 * fcn_AddHysteresis_WithinFloat, fcn_AddHysteresisMinusOffset.
 *
 * Pure logic — no hardware deps, no mocks.
 *
 * Ceedling change from Phase 1 version (tests/test_numbers.c):
 *   - No main() — Ceedling auto-discovers void test_*() functions.
 *
 * Run (from tests_ceedling/ directory):
 *   ceedling test:test_x04_Numbers
 * =============================================================================*/
#include "unity.h"
#include "x04_Numbers.h"

void setUp(void)    {}
void tearDown(void) {}

/* ===========================================================================
 * fcn_Constrain_WithinFloats
 * =========================================================================== */

void test_Constrain_ValueWithinLimits(void)
{
    float val = 50.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(NO_SATURATION, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, val);
}

void test_Constrain_ValueAboveUpper(void)
{
    float val = 150.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(POSITIVE_SATURATION, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, val);
}

/* BUG-001 — lower-branch returns POSITIVE_SATURATION instead of NEGATIVE.
 * This test verifies the Phase 1 fix is in place. */
void test_Constrain_ValueBelowLower(void)
{
    float val = -5.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(NEGATIVE_SATURATION, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, val);
}

void test_Constrain_ValueAtUpperBoundary(void)
{
    float val = 100.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(NO_SATURATION, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, val);
}

void test_Constrain_ValueAtLowerBoundary(void)
{
    float val = 0.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(NO_SATURATION, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, val);
}

/* ===========================================================================
 * fcn_ChrArrayToFloat
 * =========================================================================== */

void test_ChrArrayToFloat_93_5(void)
{
    char arr[] = {0x30, 0x39, 0x33, 0x35};
    float result = fcn_ChrArrayToFloat(arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 93.5f, result);
}

void test_ChrArrayToFloat_Zero(void)
{
    char arr[] = {0x30, 0x30, 0x30, 0x30};
    float result = fcn_ChrArrayToFloat(arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result);
}

void test_ChrArrayToFloat_130_0(void)
{
    char arr[] = {0x31, 0x33, 0x30, 0x30};
    float result = fcn_ChrArrayToFloat(arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 130.0f, result);
}

/* ===========================================================================
 * fcn_FloatToChrArray — round-trip
 * =========================================================================== */

void test_FloatToChrArray_RoundTrip_93_5(void)
{
    uint8_t arr[4] = {0};
    fcn_FloatToChrArray(93.5f, arr, 3, 1);
    float back = fcn_ChrArrayToFloat((char*)arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 93.5f, back);
}

void test_FloatToChrArray_RoundTrip_Zero(void)
{
    uint8_t arr[4] = {0};
    fcn_FloatToChrArray(0.0f, arr, 3, 1);
    float back = fcn_ChrArrayToFloat((char*)arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, back);
}

/* ===========================================================================
 * fcn_AddHysteresis_WithinFloat
 * =========================================================================== */

void test_Hysteresis_WithinBand_UpdatesToNewValue(void)
{
    float val = 1.0f;
    fcn_AddHysteresis_WithinFloat(&val, 7.5f, 2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.5f, val);
}

void test_Hysteresis_OutsideBand_NoChange(void)
{
    float val = 5.0f;
    fcn_AddHysteresis_WithinFloat(&val, 7.5f, 2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, val);
}

/* ===========================================================================
 * fcn_AddHysteresisMinusOffset
 * =========================================================================== */

void test_HysteresisMinusOffset_BelowLower_ZerosValue(void)
{
    float val = 0.5f;
    fcn_AddHysteresisMinusOffset(&val, 0.5f, 3.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, val);
}

void test_HysteresisMinusOffset_InBand_SubtractsLower(void)
{
    float val = 2.0f;
    fcn_AddHysteresisMinusOffset(&val, 2.0f, 3.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, val);
}

void test_HysteresisMinusOffset_NegativeInput_BelowLower(void)
{
    float val = -0.5f;
    fcn_AddHysteresisMinusOffset(&val, -0.5f, 3.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, val);
}
