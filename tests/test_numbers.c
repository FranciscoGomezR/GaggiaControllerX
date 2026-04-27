/*
 * test_numbers.c
 *
 * Phase 1 — TDD tests for x04_Numbers.c
 *
 * Run: make -f tests/Makefile out/test_numbers.exe && tests/out/test_numbers.exe
 *
 * Known bug caught here:
 *   test_Constrain_ValueBelowLower — both branches of fcn_Constrain_WithinFloats
 *   return POSITIVE_SATURATION; the lower-bound branch should return NEGATIVE_SATURATION.
 */

#include "unity.h"
#include "x04_Numbers.h"

/* ---- Unity boilerplate --------------------------------------------------- */
void setUp(void)    {}
void tearDown(void) {}

/* =========================================================================
 * fcn_Constrain_WithinFloats
 *
 * Implementation signature (from .c file):
 *   int8_t fcn_Constrain_WithinFloats(float* Number, float UpperLimit, float LowerLimit)
 *   2nd arg = upper limit, 3rd arg = lower limit
 * ========================================================================= */

/* Value within [0, 100] — no saturation, Number unchanged */
void test_Constrain_ValueWithinLimits(void)
{
    float val = 50.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(NO_SATURATION, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, val);
}

/* Value above upper limit — clamped to 100, returns POSITIVE_SATURATION */
void test_Constrain_ValueAboveUpper(void)
{
    float val = 150.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(POSITIVE_SATURATION, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, val);
}

/*
 * RED TEST — Known bug:
 * The lower-bound branch returns POSITIVE_SATURATION instead of NEGATIVE_SATURATION.
 * This test will FAIL until the bug in x04_Numbers.c is fixed.
 * Fix: change the second "return POSITIVE_SATURATION;" to "return NEGATIVE_SATURATION;"
 */
void test_Constrain_ValueBelowLower(void)
{
    float val = -5.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(NEGATIVE_SATURATION, result);   /* BUG: currently returns +1 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, val);
}

/* Value at exact upper boundary — no saturation */
void test_Constrain_ValueAtUpperBoundary(void)
{
    float val = 100.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(NO_SATURATION, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, val);
}

/* Value at exact lower boundary — no saturation */
void test_Constrain_ValueAtLowerBoundary(void)
{
    float val = 0.0f;
    int8_t result = fcn_Constrain_WithinFloats(&val, 100.0f, 0.0f);
    TEST_ASSERT_EQUAL_INT8(NO_SATURATION, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, val);
}

/* =========================================================================
 * fcn_ChrArrayToFloat
 *
 *   float fcn_ChrArrayToFloat(char* ptrArray, char noDigits, char noDecimals)
 *   noDigits includes decimal digit count; noDecimals is the decimal count.
 *   Array layout: [digit0][digit1]...[decimalDigit0][decimalDigit1...]
 * ========================================================================= */

/* 093.5 → digits=3 ('0','9','3'), decimals=1 ('5') → 93.5 */
void test_ChrArrayToFloat_93_5(void)
{
    char arr[] = {0x30, 0x39, 0x33, 0x35};  /* '0','9','3','5' */
    float result = fcn_ChrArrayToFloat(arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 93.5f, result);
}

/* 000.0 → 0.0 */
void test_ChrArrayToFloat_Zero(void)
{
    char arr[] = {0x30, 0x30, 0x30, 0x30};  /* '0','0','0','0' */
    float result = fcn_ChrArrayToFloat(arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result);
}

/* 130.0 → digits=3 ('1','3','0'), decimals=1 ('0') → 130.0 */
void test_ChrArrayToFloat_130_0(void)
{
    char arr[] = {0x31, 0x33, 0x30, 0x30};  /* '1','3','0','0' */
    float result = fcn_ChrArrayToFloat(arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 130.0f, result);
}

/* =========================================================================
 * fcn_FloatToChrArray + round-trip
 *
 *   void fcn_FloatToChrArray(float fNum, uint8_t* ptrArray, char noDigits, char noDecimals)
 *   noDecimals must be 1 (only case 1 is implemented).
 * ========================================================================= */

/* Round-trip: encode 93.5 → decode → 93.5 */
void test_FloatToChrArray_RoundTrip_93_5(void)
{
    uint8_t arr[4] = {0};
    fcn_FloatToChrArray(93.5f, arr, 3, 1);
    float back = fcn_ChrArrayToFloat((char*)arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 93.5f, back);
}

/* Round-trip: encode 0.0 → decode → 0.0 */
void test_FloatToChrArray_RoundTrip_Zero(void)
{
    uint8_t arr[4] = {0};
    fcn_FloatToChrArray(0.0f, arr, 3, 1);
    float back = fcn_ChrArrayToFloat((char*)arr, 3, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, back);
}

/* =========================================================================
 * fcn_AddHysteresis_WithinFloat
 *
 *   void fcn_AddHysteresis_WithinFloat(float* Number, float NumberWithoutHyst, float OffsetLimit)
 *   If |*Number| <= OffsetLimit: *Number = NumberWithoutHyst
 *   Otherwise: *Number is left unchanged.
 * ========================================================================= */

/* Input within dead-band (-2..+2): output set to NumberWithoutHyst */
void test_Hysteresis_WithinBand_UpdatesToNewValue(void)
{
    float val = 1.0f;                   /* within ±2.0 band */
    fcn_AddHysteresis_WithinFloat(&val, 7.5f, 2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.5f, val);
}

/* Input outside band (|val| > OffsetLimit): output unchanged */
void test_Hysteresis_OutsideBand_NoChange(void)
{
    float val = 5.0f;                   /* outside ±2.0 band */
    fcn_AddHysteresis_WithinFloat(&val, 7.5f, 2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, val);
}

/* =========================================================================
 * fcn_AddHysteresisMinusOffset
 *
 *   void fcn_AddHysteresisMinusOffset(float* Number, float NumberWithoutHyst,
 *                                     float OffsetUpperLimit, float OffsetLowerLimit)
 *   Positive input: if |*Number| <= OffsetLowerLimit → 0; else subtract lower offset
 *   Negative input: same math on abs, then negate result
 * ========================================================================= */

/* Positive val below lower offset → zeroed */
void test_HysteresisMinusOffset_BelowLower_ZerosValue(void)
{
    float val = 0.5f;
    fcn_AddHysteresisMinusOffset(&val, 0.5f, 3.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, val);
}

/* Positive val between lower and upper offsets → subtract lower */
void test_HysteresisMinusOffset_InBand_SubtractsLower(void)
{
    float val = 2.0f;   /* OffsetLower=1.0, OffsetUpper=3.0 */
    fcn_AddHysteresisMinusOffset(&val, 2.0f, 3.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, val);  /* 2.0 - 1.0 = 1.0 */
}

/* Negative input — works on abs value then negates */
void test_HysteresisMinusOffset_NegativeInput_BelowLower(void)
{
    float val = -0.5f;
    fcn_AddHysteresisMinusOffset(&val, -0.5f, 3.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, val);  /* abs(-0.5)=0.5 < 1.0 → 0, negate → 0 */
}

/* =========================================================================
 * Test runner
 * ========================================================================= */
int main(void)
{
    UNITY_BEGIN();

    /* fcn_Constrain_WithinFloats */
    RUN_TEST(test_Constrain_ValueWithinLimits);
    RUN_TEST(test_Constrain_ValueAboveUpper);
    RUN_TEST(test_Constrain_ValueBelowLower);       /* RED — exposes known bug */
    RUN_TEST(test_Constrain_ValueAtUpperBoundary);
    RUN_TEST(test_Constrain_ValueAtLowerBoundary);

    /* fcn_ChrArrayToFloat */
    RUN_TEST(test_ChrArrayToFloat_93_5);
    RUN_TEST(test_ChrArrayToFloat_Zero);
    RUN_TEST(test_ChrArrayToFloat_130_0);

    /* fcn_FloatToChrArray */
    RUN_TEST(test_FloatToChrArray_RoundTrip_93_5);
    RUN_TEST(test_FloatToChrArray_RoundTrip_Zero);

    /* fcn_AddHysteresis_WithinFloat */
    RUN_TEST(test_Hysteresis_WithinBand_UpdatesToNewValue);
    RUN_TEST(test_Hysteresis_OutsideBand_NoChange);

    /* fcn_AddHysteresisMinusOffset */
    RUN_TEST(test_HysteresisMinusOffset_BelowLower_ZerosValue);
    RUN_TEST(test_HysteresisMinusOffset_InBand_SubtractsLower);
    RUN_TEST(test_HysteresisMinusOffset_NegativeInput_BelowLower);

    return UNITY_END();
}
