/*
 * test_pid_block.c
 *
 * Phase 1 — TDD tests for x205_PID_Block.c
 * Covers: fcn_update_PID_Block, fcn_PID_Block_ResetI,
 *         fcn_update_PIDimc_typeA, fcn_update_PIDimc_typeB
 *
 * Run: make -f tests/Makefile out/test_pid_block.exe && tests/out/test_pid_block.exe
 *
 * Notes on static Block vars inside typeA/typeB:
 *   fcn_update_PIDimc_typeA and _typeB each hold a `static` internal struct.
 *   To get deterministic dt on the first real call in a test, set prevT_Milis
 *   to the initial TimeMilis value before the first call (dt = 0 → safe no-op
 *   for I/D, only P fires). Then advance TimeMilis for subsequent calls.
 */

#include <math.h>
#include "unity.h"
#include "x04_Numbers.h"    /* POSITIVE_SATURATION, NEGATIVE_SATURATION, NO_SATURATION */
#include "x205_PID_Block.h"

/* ---- Unity boilerplate --------------------------------------------------- */
void setUp(void)    {}
void tearDown(void) {}

/* =========================================================================
 * Helper: zero-init a PID_Block_fStruct with sensible defaults
 * ========================================================================= */
static void pid_block_reset(PID_Block_fStruct* p,
                             float kp, float ki, float kd,
                             float out_limit, float int_limit)
{
    /* Cast to void* + memset not available easily; use field-by-field */
    p->feedPIDblock.ProcessVariable = 0.0f;
    p->feedPIDblock.SetPoint        = 0.0f;
    p->feedPIDblock.TimeMilis       = 0;
    p->PrevError    = 0.0f;
    p->prevT_Milis  = 0.0f;
    p->Output       = 0.0f;
    p->OutputLimit  = out_limit;
    p->OutputSaturationOut = 0;

    p->P_TERM_CTRL  = (kp != 0.0f);
    p->Kp           = kp;

    p->I_TERM_CTRL  = (ki != 0.0f);
    p->Ki           = ki;
    p->HistoryError = 0.0f;
    p->IntegralLimit = int_limit;
    p->I_ANTIWINDUP_CTRL  = 0;
    p->WindupClampStatus  = 0;

    p->D_TERM_CTRL  = (kd != 0.0f);
    p->Kd           = kd;
    p->D_TERM_LP_FILTER_CTRL = 0;
}

/* Helper: zero-init a PID_IMC_Block_fStruct */
static void pid_imc_reset(PID_IMC_Block_fStruct* p,
                           float kp, float ki, float kd,
                           float out_limit, float int_limit)
{
    p->feedPIDblock.ProcessVariable = 0.0f;
    p->feedPIDblock.SetPoint        = 0.0f;
    p->feedPIDblock.TimeMilis       = 0;
    p->prevT_Milis   = 0.0f;
    p->errorK_1      = 0.0f;
    p->errorK_2      = 0.0f;
    p->Output        = 0.0f;
    p->OutputLimit   = out_limit;
    p->OutputSaturationOut = 0;

    p->P_TERM_CTRL   = (kp != 0.0f);
    p->Kp            = kp;

    p->I_TERM_CTRL   = (ki != 0.0f);
    p->I_ANTIWINDUP_CTRL = 0;
    p->Ki            = ki;
    p->HistoryError  = 0.0f;
    p->IntegralError = 0.0f;
    p->IntegralLimit = int_limit;
    p->WindupClampStatus = 0;

    p->D_TERM_CTRL   = (kd != 0.0f);
    p->Kd            = kd;
    p->prevPV        = 0.0f;
}

/* =========================================================================
 * fcn_update_PID_Block — P-only test
 * ========================================================================= */

/*
 * P-only: SP=100, PV=90, Kp=2.0, Ki=0, Kd=0
 * error = 10, P_Term = 20.0, dt=10ms
 * Expected output = 20.0
 */
void test_PID_Block_Ponly_OutputEqualsKpTimesError(void)
{
    PID_Block_fStruct pid;
    pid_block_reset(&pid, 2.0f, 0.0f, 0.0f, 1000.0f, 500.0f);

    /* First call: dt = (10-0)/1000 = 0.01s, error = SP-PV = 10 */
    float out = fcn_update_PID_Block(90.0f, 100.0f, 10, &pid);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 20.0f, out);
}

/* =========================================================================
 * fcn_update_PID_Block — I-only accumulation
 * ========================================================================= */

/*
 * I-only: Kp=0, Ki=1.0, Kd=0, constant error=10, dt=100ms each step
 * After N steps: HistoryError ≈ 10 * 0.1 * N
 * After 5 steps: HistoryError ≈ 5.0, I_Term = 5.0 * 1.0 = 5.0
 * But integral clamping via I_Term (not HistoryError) may truncate.
 * With IntegralLimit=500 (large), no clamping expected.
 */
void test_PID_Block_Ionly_AccumulatesLinearly(void)
{
    PID_Block_fStruct pid;
    pid_block_reset(&pid, 0.0f, 1.0f, 0.0f, 1000.0f, 500.0f);

    const float dt_ms   = 100.0f;  /* 100ms per step */
    const float error   = 10.0f;
    const int   N       = 5;

    for (int i = 1; i <= N; i++) {
        fcn_update_PID_Block(0.0f, error, (uint32_t)(i * dt_ms), &pid);
    }

    /* HistoryError ≈ 10 * 0.1 * 5 = 5.0 */
    float expected_history = error * (dt_ms / 1000.0f) * N;
    TEST_ASSERT_FLOAT_WITHIN(0.1f, expected_history, pid.HistoryError);
}

/* =========================================================================
 * fcn_update_PID_Block — Output saturation
 * ========================================================================= */

/*
 * Very large error with OutputLimit=100:
 * P-only, Kp=100, error=1000 → raw = 100000, clamped to 100
 */
void test_PID_Block_OutputSaturation_ClampsToLimit(void)
{
    PID_Block_fStruct pid;
    pid_block_reset(&pid, 100.0f, 0.0f, 0.0f, 100.0f, 500.0f);

    float out = fcn_update_PID_Block(0.0f, 1000.0f, 10, &pid);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(100.0f + 0.001f, out);
    TEST_ASSERT_EQUAL_INT8(POSITIVE_SATURATION, pid.OutputSaturationOut);
}

/* =========================================================================
 * fcn_PID_Block_ResetI
 * ========================================================================= */

/* Attenuator=0.5 → HistoryError halved */
void test_PID_Block_ResetI_HalvesIntegral(void)
{
    PID_Block_fStruct pid;
    pid_block_reset(&pid, 0.0f, 1.0f, 0.0f, 1000.0f, 500.0f);

    /* Accumulate some integral: 5 steps × error=10 × dt=0.1s */
    for (int i = 1; i <= 5; i++) {
        fcn_update_PID_Block(0.0f, 10.0f, (uint32_t)(i * 100), &pid);
    }
    float before = pid.HistoryError;
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, before);

    fcn_PID_Block_ResetI(&pid, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(fabsf(before) * 0.01f + 0.001f,
                              before * 0.5f, pid.HistoryError);
}

/* Attenuator=0.0 → HistoryError zeroed */
void test_PID_Block_ResetI_ZeroAttenuator(void)
{
    PID_Block_fStruct pid;
    pid_block_reset(&pid, 0.0f, 1.0f, 0.0f, 1000.0f, 500.0f);

    for (int i = 1; i <= 5; i++) {
        fcn_update_PID_Block(0.0f, 10.0f, (uint32_t)(i * 100), &pid);
    }
    fcn_PID_Block_ResetI(&pid, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pid.HistoryError);
}

/* =========================================================================
 * fcn_update_PIDimc_typeA
 * ========================================================================= */

/*
 * SP > PV → error > 0 → P output > 0.
 * Kp=2.0, Ki=0, Kd=0, SP=100, PV=80, dt=10ms
 * error=20, P_Term=40 → output ≈ 40
 */
void test_PIDimc_TypeA_HeatOnPositiveError(void)
{
    PID_IMC_Block_fStruct pid;
    pid_imc_reset(&pid, 2.0f, 0.0f, 0.0f, 1000.0f, 500.0f);

    pid.feedPIDblock.SetPoint        = 100.0f;
    pid.feedPIDblock.ProcessVariable = 80.0f;
    pid.feedPIDblock.TimeMilis       = 10;
    pid.prevT_Milis                  = 0.0f;

    float out = fcn_update_PIDimc_typeA(&pid);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, out);
}

/*
 * SP < PV → error < 0 → output ≤ 0.
 * Kp-only, SP=80, PV=100, error=-20
 */
void test_PIDimc_TypeA_CoolOnNegativeError(void)
{
    PID_IMC_Block_fStruct pid;
    pid_imc_reset(&pid, 2.0f, 0.0f, 0.0f, 1000.0f, 500.0f);

    pid.feedPIDblock.SetPoint        = 80.0f;
    pid.feedPIDblock.ProcessVariable = 100.0f;
    pid.feedPIDblock.TimeMilis       = 10;
    pid.prevT_Milis                  = 0.0f;

    float out = fcn_update_PIDimc_typeA(&pid);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(0.0f, out);
}

/*
 * Output clamped to OutputLimit when error is huge.
 * Kp=1000, SP=1000, PV=0, OutputLimit=500
 */
void test_PIDimc_TypeA_OutputClampedToLimit(void)
{
    PID_IMC_Block_fStruct pid;
    pid_imc_reset(&pid, 1000.0f, 0.0f, 0.0f, 500.0f, 500.0f);

    pid.feedPIDblock.SetPoint        = 1000.0f;
    pid.feedPIDblock.ProcessVariable = 0.0f;
    pid.feedPIDblock.TimeMilis       = 10;
    pid.prevT_Milis                  = 0.0f;

    float out = fcn_update_PIDimc_typeA(&pid);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(500.0f + 0.001f, out);
    TEST_ASSERT_EQUAL_INT8(POSITIVE_SATURATION, pid.OutputSaturationOut);
}

/* =========================================================================
 * fcn_update_PIDimc_typeB — D-term on PV (kick-free on SP step)
 * ========================================================================= */

/*
 * TypeA uses error derivative for D: reacts to SP step.
 * TypeB uses -PV derivative for D: does NOT react to SP step if PV unchanged.
 *
 * Comparison test:
 *   - Run both typeA and typeB for several identical SP/PV steps.
 *   - Then introduce an SP step (SP changes, PV stays constant).
 *   - TypeA D_Term will change due to the SP-driven error delta.
 *   - TypeB D_Term will be 0 (PV unchanged) → typeB output will differ.
 *
 * With Kd=5.0, large SP step → assertable difference in outputs.
 */
void test_PIDimc_TypeA_vs_TypeB_DifferentDterm_OnSPstep(void)
{
    PID_IMC_Block_fStruct pidA, pidB;
    pid_imc_reset(&pidA, 1.0f, 0.0f, 5.0f, 2000.0f, 500.0f);
    pid_imc_reset(&pidB, 1.0f, 0.0f, 5.0f, 2000.0f, 500.0f);

    const float PV = 90.0f;

    /* Warm-up: 3 ticks at SP=93, PV=90 (same for both) */
    for (int i = 1; i <= 3; i++) {
        pidA.feedPIDblock.SetPoint        = 93.0f;
        pidA.feedPIDblock.ProcessVariable = PV;
        pidA.feedPIDblock.TimeMilis       = (uint32_t)(i * 10);

        pidB.feedPIDblock.SetPoint        = 93.0f;
        pidB.feedPIDblock.ProcessVariable = PV;
        pidB.feedPIDblock.TimeMilis       = (uint32_t)(i * 10);

        fcn_update_PIDimc_typeA(&pidA);
        fcn_update_PIDimc_typeB(&pidB);
    }

    /* SP step: both see new SP=150, PV unchanged at 90 */
    pidA.feedPIDblock.SetPoint        = 150.0f;
    pidA.feedPIDblock.ProcessVariable = PV;
    pidA.feedPIDblock.TimeMilis       = 40;

    pidB.feedPIDblock.SetPoint        = 150.0f;
    pidB.feedPIDblock.ProcessVariable = PV;
    pidB.feedPIDblock.TimeMilis       = 40;

    float outA = fcn_update_PIDimc_typeA(&pidA);
    float outB = fcn_update_PIDimc_typeB(&pidB);

    /*
     * TypeA D reacts to error spike from SP step → outA contains a large D kick.
     * TypeB D = -(PV - prevPV)/dt → PV unchanged → D=0 → outB has no D kick.
     * Assert they differ by at least 1.0 (very conservative given Kd=5 and a
     * 57-unit SP step producing ~28500 raw D in typeA).
     */
    TEST_ASSERT_NOT_EQUAL(0, fabsf(outA - outB) > 1.0f);
}

/* =========================================================================
 * Test runner
 * ========================================================================= */
int main(void)
{
    UNITY_BEGIN();

    /* fcn_update_PID_Block */
    RUN_TEST(test_PID_Block_Ponly_OutputEqualsKpTimesError);
    RUN_TEST(test_PID_Block_Ionly_AccumulatesLinearly);
    RUN_TEST(test_PID_Block_OutputSaturation_ClampsToLimit);

    /* fcn_PID_Block_ResetI */
    RUN_TEST(test_PID_Block_ResetI_HalvesIntegral);
    RUN_TEST(test_PID_Block_ResetI_ZeroAttenuator);

    /* fcn_update_PIDimc_typeA */
    RUN_TEST(test_PIDimc_TypeA_HeatOnPositiveError);
    RUN_TEST(test_PIDimc_TypeA_CoolOnNegativeError);
    RUN_TEST(test_PIDimc_TypeA_OutputClampedToLimit);

    /* fcn_update_PIDimc_typeB vs typeA */
    RUN_TEST(test_PIDimc_TypeA_vs_TypeB_DifferentDterm_OnSPstep);

    return UNITY_END();
}
