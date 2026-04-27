/*
 * test_digital_filters.c
 *
 * Phase 1 — TDD tests for x201_DigitalFiltersAlgorithm.c
 *
 * Run: make -f tests/Makefile out/test_digital_filters.exe && tests/out/test_digital_filters.exe
 *
 * Tests cover both fixed-Ts and variable-Ts RC low-pass filter paths.
 */

#include <math.h>
#include "unity.h"
#include "x201_DigitalFiltersAlgorithm.h"

/* ---- Unity boilerplate --------------------------------------------------- */
void setUp(void)    {}
void tearDown(void) {}

/* =========================================================================
 * pfcn_InitRCFilterAlgorithm + pfcn_RCFilterAlgorithm (fixed Ts)
 * ========================================================================= */

/*
 * Init with Fc=10 Hz, Ts=0.01 s:
 *   RC = 1/(2π*10) ≈ 0.01592
 *   coeff[0] = Ts/(Ts+RC) = 0.01/(0.01+0.01592) ≈ 0.3857  (alpha)
 *   coeff[1] = RC/(RC+Ts) ≈ 0.6143  (1-alpha)
 *   Both non-zero, coeff[0]+coeff[1] = 1.0
 */
void test_RCFilter_Fixed_Init_SetsCoefficients(void)
{
    lpf_rc_param_t f = {0};
    pfcn_InitRCFilterAlgorithm(&f, 10.0f, 0.01f);

    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, f.FilterRCCoefficients[0]);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, f.FilterRCCoefficients[1]);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, f.FilterRCCoefficients[0]);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, f.FilterRCCoefficients[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f,
        f.FilterRCCoefficients[0] + f.FilterRCCoefficients[1]);
}

/* DC convergence: constant input 5.0, 200 iterations → output ≈ 5.0 */
void test_RCFilter_Fixed_DCConvergence(void)
{
    lpf_rc_param_t f = {0};
    pfcn_InitRCFilterAlgorithm(&f, 10.0f, 0.01f);
    for (int i = 0; i < 200; i++) {
        pfcn_RCFilterAlgorithm(&f, 5.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 5.0f, f.DataOut_n);
}

/* Zero input: output remains 0 */
void test_RCFilter_Fixed_ZeroInput_StaysZero(void)
{
    lpf_rc_param_t f = {0};
    pfcn_InitRCFilterAlgorithm(&f, 10.0f, 0.01f);
    for (int i = 0; i < 100; i++) {
        pfcn_RCFilterAlgorithm(&f, 0.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, f.DataOut_n);
}

/* Step response attenuates: after 1 iteration output < input */
void test_RCFilter_Fixed_StepResponse_Attenuates(void)
{
    lpf_rc_param_t f = {0};
    pfcn_InitRCFilterAlgorithm(&f, 10.0f, 0.01f);
    pfcn_RCFilterAlgorithm(&f, 1.0f);
    TEST_ASSERT_LESS_THAN_FLOAT(1.0f, f.DataOut_n);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, f.DataOut_n);
}

/* =========================================================================
 * lpf_rc_calculate_const + lpf_rc_update (variable Ts)
 * ========================================================================= */

/*
 * lpf_rc_calculate_const: rc_constant = 1/(2π*Fc)
 * For Fc=1 Hz: expected = 1/(2π) ≈ 0.15915
 */
void test_RCFilter_Variable_CalculatesConst(void)
{
    lpf_rc_param_t f = {0};
    lpf_rc_calculate_const(&f, 1.0f);
    float expected = 1.0f / (2.0f * 3.14159265f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, f.rc_constant);
}

/*
 * Step response at Fc=1 Hz, dt=0.001 s:
 *   τ = 1/(2π*1) ≈ 0.1592 s
 *   After N = τ/dt = 159 iterations, output should reach ≥ 63.2% of input.
 */
void test_RCFilter_Variable_StepResponse_63pct(void)
{
    lpf_rc_param_t f = {0};
    lpf_rc_calculate_const(&f, 1.0f);

    const float dt  = 0.001f;
    const float tau = 1.0f / (2.0f * 3.14159265f * 1.0f);
    int n_tau       = (int)(tau / dt) + 1;  /* iterations for 1 tau */

    float out = 0.0f;
    for (int i = 0; i < n_tau; i++) {
        out = lpf_rc_update(&f, 1.0f, dt);
    }
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.632f, out);
}

/* Zero input, variable Ts: output stays 0 */
void test_RCFilter_Variable_ZeroInput_StaysZero(void)
{
    lpf_rc_param_t f = {0};
    lpf_rc_calculate_const(&f, 10.0f);
    for (int i = 0; i < 100; i++) {
        lpf_rc_update(&f, 0.0f, 0.001f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, f.DataOut_n);
}

/*
 * Consistency: fixed-Ts and variable-Ts with same Fc and constant dt
 * should produce matching outputs within 0.1%.
 */
void test_RCFilter_Fixed_vs_Variable_Consistent(void)
{
    const float Fc   = 5.0f;
    const float Ts   = 0.01f;
    const float input = 3.0f;
    const int   N    = 50;

    lpf_rc_param_t fixed  = {0};
    lpf_rc_param_t varTs  = {0};
    pfcn_InitRCFilterAlgorithm(&fixed, Fc, Ts);
    lpf_rc_calculate_const(&varTs, Fc);

    float out_fixed = 0.0f;
    float out_var   = 0.0f;
    for (int i = 0; i < N; i++) {
        pfcn_RCFilterAlgorithm(&fixed, input);
        out_fixed = fixed.DataOut_n;
        out_var   = lpf_rc_update(&varTs, input, Ts);
    }
    TEST_ASSERT_FLOAT_WITHIN(out_fixed * 0.001f + 0.001f, out_fixed, out_var);
}

/* =========================================================================
 * Test runner
 * ========================================================================= */
int main(void)
{
    UNITY_BEGIN();

    /* Fixed-Ts */
    RUN_TEST(test_RCFilter_Fixed_Init_SetsCoefficients);
    RUN_TEST(test_RCFilter_Fixed_DCConvergence);
    RUN_TEST(test_RCFilter_Fixed_ZeroInput_StaysZero);
    RUN_TEST(test_RCFilter_Fixed_StepResponse_Attenuates);

    /* Variable-Ts */
    RUN_TEST(test_RCFilter_Variable_CalculatesConst);
    RUN_TEST(test_RCFilter_Variable_StepResponse_63pct);
    RUN_TEST(test_RCFilter_Variable_ZeroInput_StaysZero);
    RUN_TEST(test_RCFilter_Fixed_vs_Variable_Consistent);

    return UNITY_END();
}
