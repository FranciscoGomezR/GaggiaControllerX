/* =============================================================================
 * test_temp_controller.c — Phase 2 TDD tests for tempController.c
 *
 * Issues covered:
 *   H3 (Green): Sensor out-of-range shuts heater off — fixed in this session.
 *   M1 (Green): Setpoint change resets integral — fixed in this session.
 *
 * Compile (from project root):
 *   gcc ... tempController.c x205_PID_Block.c x201_DigitalFiltersAlgorithm.c
 *           x04_Numbers.c unity.c test_temp_controller.c -lm -o test_tc.exe
 * =============================================================================*/
#include "unity.h"

/* fakes.h provides DEFINE_FFF_GLOBALS and hardware-driver fakes.
 * We do NOT define tempController fakes here because we compile the real
 * tempController.c into this test binary. */
#include "fakes.h"

/* Module under test */
#include "tempController.h"

/* Test-only helpers injected via #ifdef TEST guards in tempController.c */
void     test_set_milisTicks(uint32_t t);
uint32_t test_get_milisTicks(void);
float    test_get_integral_error(void);

/* blEspressoProfile is defined in BLEspressoServices.c, which is NOT compiled
 * in this test binary. Provide a test-local definition. */
volatile bleSpressoUserdata_struct blEspressoProfile;

/* ---------------------------------------------------------------------------
 * setUp / tearDown — run before/after every test
 * --------------------------------------------------------------------------- */
void setUp(void)
{
    RESET_ALL_HW_FAKES();
    test_set_milisTicks(0);

    /* Default profile values used in most tests */
    blEspressoProfile.Pid_P_term      = 9.5f;
    blEspressoProfile.Pid_I_term      = 0.1f;
    blEspressoProfile.Pid_Iboost_term = 0.3f;
    blEspressoProfile.Pid_Imax_term   = 200.0f;
    blEspressoProfile.Pid_Iwindup_term= true;
    blEspressoProfile.Pid_D_term      = 0.0f;
    blEspressoProfile.sp_BrewTemp     = 93.0f;
    blEspressoProfile.sp_StemTemp     = 130.0f;
    blEspressoProfile.temp_Target     = 93.0f;
    blEspressoProfile.temp_Boiler     = 20.0f;
}

void tearDown(void) {}

/* ---------------------------------------------------------------------------
 * Test 1 — Init returns OK
 * --------------------------------------------------------------------------- */
void test_TC_Init_Returns_OK(void)
{
    tempCtrl_status_t result = fcn_initCntrl_Temp();
    TEST_ASSERT_EQUAL_INT(TEMPCTRL_INIT_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 2 — Load PID parameters returns OK
 * --------------------------------------------------------------------------- */
void test_TC_LoadPID_Valid_Returns_OK(void)
{
    blEspressoProfile.Pid_P_term = 9.52156f;
    blEspressoProfile.Pid_I_term = 0.3f;
    blEspressoProfile.Pid_D_term = 0.0f;
    blEspressoProfile.Pid_Imax_term  = 100.0f;
    blEspressoProfile.Pid_Iwindup_term = false;

    tempCtrl_status_t result = fcn_loadPID_ParamToCtrl_Temp(
                                    (bleSpressoUserdata_struct *)&blEspressoProfile);
    TEST_ASSERT_EQUAL_INT(TEMPCTRL_LOAD_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 3 — Setting P-term to 0 should disable the P term (verified by
 *           output: with only I and D = 0, a single tick should produce 0
 *           output if P is disabled and integral has not yet accumulated).
 * --------------------------------------------------------------------------- */
void test_TC_LoadPID_P_Zero_Disables_P_Control(void)
{
    /* Disable P, keep I small, D = 0 */
    blEspressoProfile.Pid_P_term      = 0.0f;
    blEspressoProfile.Pid_I_term      = 0.0f;   /* also disable I */
    blEspressoProfile.Pid_D_term      = 0.0f;
    blEspressoProfile.Pid_Imax_term   = 0.0f;
    blEspressoProfile.Pid_Iwindup_term= false;

    tempCtrl_status_t result = fcn_loadPID_ParamToCtrl_Temp(
                                    (bleSpressoUserdata_struct *)&blEspressoProfile);
    TEST_ASSERT_EQUAL_INT(TEMPCTRL_LOAD_OK, (int)result);

    /* With all terms disabled, one PID tick should return 0 */
    blEspressoProfile.temp_Boiler = 20.0f;
    blEspressoProfile.temp_Target = 93.0f;
    test_set_milisTicks(1000);
    float output = fcn_updateTemperatureController(
                        (bleSpressoUserdata_struct *)&blEspressoProfile);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

/* ---------------------------------------------------------------------------
 * Test 4 — Load setpoint BREW sets temp_Target to sp_BrewTemp
 * --------------------------------------------------------------------------- */
void test_TC_SetPoint_Brew_Sets_BrewTemp(void)
{
    blEspressoProfile.sp_BrewTemp  = 93.0f;
    blEspressoProfile.temp_Target  = 130.0f;   /* start at steam value */

    tempCtrl_LoadSP_t result = fcn_loaddSetPoint_ParamToCtrl_Temp(
                                    (bleSpressoUserdata_struct *)&blEspressoProfile,
                                    SETPOINT_BREW);
    TEST_ASSERT_EQUAL_INT(TEMPCTRL_SP_LOAD_OK, (int)result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f, blEspressoProfile.temp_Target);
}

/* ---------------------------------------------------------------------------
 * Test 5 — Load setpoint STEAM sets temp_Target to sp_StemTemp
 * --------------------------------------------------------------------------- */
void test_TC_SetPoint_Steam_Sets_SteamTemp(void)
{
    blEspressoProfile.sp_StemTemp = 130.0f;
    blEspressoProfile.temp_Target =  93.0f;

    tempCtrl_LoadSP_t result = fcn_loaddSetPoint_ParamToCtrl_Temp(
                                    (bleSpressoUserdata_struct *)&blEspressoProfile,
                                    SETPOINT_STEAM);
    TEST_ASSERT_EQUAL_INT(TEMPCTRL_SP_LOAD_OK, (int)result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 130.0f, blEspressoProfile.temp_Target);
}

/* ---------------------------------------------------------------------------
 * Test 6 — I-boost load returns OK
 * --------------------------------------------------------------------------- */
void test_TC_IBoost_Load_Returns_OK(void)
{
    blEspressoProfile.Pid_Iboost_term = 0.65f;
    tempCtrl_status_t result = fcn_loadIboost_ParamToCtrl_Temp(
                                    (bleSpressoUserdata_struct *)&blEspressoProfile);
    TEST_ASSERT_EQUAL_INT(TEMPCTRL_I_LOAD_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 7 — Multiply I returns OK
 * --------------------------------------------------------------------------- */
void test_TC_MultiplyI_Returns_OK(void)
{
    tempCtrl_status_t result = fcn_multiplyI_ParamToCtrl_Temp(
                                    (bleSpressoUserdata_struct *)&blEspressoProfile,
                                    2.0f);
    TEST_ASSERT_EQUAL_INT(TEMPCTRL_I_LOAD_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 8 — Normal operation: SP > PV should produce positive PID output.
 *           Uses P+I only (D=0) to avoid NaN from D/dt when dt=0.
 * --------------------------------------------------------------------------- */
void test_TC_Update_Normal_Positive_Output(void)
{
    fcn_initCntrl_Temp();

    /* Explicitly set profile and load it so D_TERM is disabled (Kd=0 maps to
     * NOT_ACTIVE in fcn_loadPID_ParamToCtrl_Temp).  This avoids a latent
     * D_Term/dt NaN when dt=0 because prevT_Milis == milisTicks. */
    blEspressoProfile.Pid_P_term      = 9.52156f;
    blEspressoProfile.Pid_I_term      = 0.3f;
    blEspressoProfile.Pid_Imax_term   = 200.0f;
    blEspressoProfile.Pid_Iwindup_term= false;
    blEspressoProfile.Pid_D_term      = 0.0f;   /* D=0 → NOT_ACTIVE */
    fcn_loadPID_ParamToCtrl_Temp((bleSpressoUserdata_struct *)&blEspressoProfile);

    blEspressoProfile.temp_Boiler = 20.0f;   /* cold boiler */
    blEspressoProfile.temp_Target = 93.0f;   /* brew setpoint */

    /* Use a timestamp far beyond any prevT_Milis from previous tests so
     * dt is guaranteed positive. */
    test_set_milisTicks(60000);
    float output = fcn_updateTemperatureController(
                        (bleSpressoUserdata_struct *)&blEspressoProfile);

    /* With a large positive error (93 - 20 = 73 °C) the PID must command
     * positive power. Output is clamped between 0 and 1000. */
    TEST_ASSERT_TRUE(output > 0.0f);
    TEST_ASSERT_TRUE(output <= 1000.0f);
}

/* ---------------------------------------------------------------------------
 * Test 9 — H3 (Green): open-circuit sensor (0 °C reading) must not command
 *           heater power — instead fcn_updateTemperatureController returns 0.
 *
 * Rationale: Without protection, an open RTD reads ~0 °C, creating a
 *             huge positive error (93 - 0 = 93 °C) that drives the heater
 *             to 100 %. This fix detects the out-of-range reading and returns 0.
 * --------------------------------------------------------------------------- */
void test_H3_OpenSensor_ShutoffHeater(void)
{
    fcn_initCntrl_Temp();
    blEspressoProfile.temp_Boiler = 0.0f;   /* open-circuit sensor reading */
    blEspressoProfile.temp_Target = 93.0f;
    test_set_milisTicks(1000);

    float output = fcn_updateTemperatureController(
                        (bleSpressoUserdata_struct *)&blEspressoProfile);

    /* H3 fix active: sensor out of range → return 0, no heating */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, output);
}

/* ---------------------------------------------------------------------------
 * Test 10 — H3 (Green): shorted sensor (> 200 °C reading) must also shut
 *            the heater off.
 * --------------------------------------------------------------------------- */
void test_H3_ShortedSensor_ShutoffHeater(void)
{
    fcn_initCntrl_Temp();
    blEspressoProfile.temp_Boiler = 400.0f;  /* shorted sensor */
    blEspressoProfile.temp_Target =  93.0f;
    test_set_milisTicks(1000);

    float output = fcn_updateTemperatureController(
                        (bleSpressoUserdata_struct *)&blEspressoProfile);

    /* Fix: shorted sensor detected, return 0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, output);
}

/* ---------------------------------------------------------------------------
 * Test 11 — M1 (Green): switching the setpoint must reset the integral
 *           accumulator so that large historical errors from the previous
 *           operating mode do not bleed into the new mode.
 * --------------------------------------------------------------------------- */
void test_M1_SetpointChange_ResetsIntegral(void)
{
    /* Reset PID to a known state with P=0 so only the integral accumulates.
     * D=0 avoids the D/dt NaN risk.  Use a large Imax so the clamp does not
     * limit the integral during the accumulation loop. */
    blEspressoProfile.Pid_P_term       = 0.0f;
    blEspressoProfile.Pid_I_term       = 0.05f;
    blEspressoProfile.Pid_Imax_term    = 5000.0f;
    blEspressoProfile.Pid_Iwindup_term = false;
    blEspressoProfile.Pid_D_term       = 0.0f;
    fcn_initCntrl_Temp();
    fcn_loadPID_ParamToCtrl_Temp((bleSpressoUserdata_struct *)&blEspressoProfile);

    /* Seed prevT_Milis by making one "warm-up" PID call with a very high
     * timestamp so the first accumulation tick has a well-defined dt = 1 s.
     * Using 200000 ms (200 s) ensures it is above any timestamp used in
     * earlier test cases. */
    blEspressoProfile.temp_Boiler = 70.0f;
    blEspressoProfile.temp_Target = 130.0f;
    test_set_milisTicks(200000);
    (void)fcn_updateTemperatureController(
                        (bleSpressoUserdata_struct *)&blEspressoProfile);

    /* Run 40 ticks (each 1 s) to accumulate a positive integral.
     * error = SP(130) − PV(70) = +60, so integral grows positive. */
    for (uint32_t i = 1; i <= 40; i++) {
        test_set_milisTicks(200000 + i * 1000);
        (void)fcn_updateTemperatureController(
                        (bleSpressoUserdata_struct *)&blEspressoProfile);
    }

    float integral_before = test_get_integral_error();
    /* Verify integral actually built up in the positive direction */
    TEST_ASSERT_TRUE(integral_before > 0.0f);

    /* Switch setpoint to brew — M1 fix resets integral inside this call */
    blEspressoProfile.sp_BrewTemp = 93.0f;
    fcn_loaddSetPoint_ParamToCtrl_Temp(
                    (bleSpressoUserdata_struct *)&blEspressoProfile, SETPOINT_BREW);

    float integral_after = test_get_integral_error();
    /* M1 fix active: integral must be zero after setpoint change */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, integral_after);
}

/* ---------------------------------------------------------------------------
 * Unity test runner
 * --------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_TC_Init_Returns_OK);
    RUN_TEST(test_TC_LoadPID_Valid_Returns_OK);
    RUN_TEST(test_TC_LoadPID_P_Zero_Disables_P_Control);
    RUN_TEST(test_TC_SetPoint_Brew_Sets_BrewTemp);
    RUN_TEST(test_TC_SetPoint_Steam_Sets_SteamTemp);
    RUN_TEST(test_TC_IBoost_Load_Returns_OK);
    RUN_TEST(test_TC_MultiplyI_Returns_OK);
    RUN_TEST(test_TC_Update_Normal_Positive_Output);
    RUN_TEST(test_H3_OpenSensor_ShutoffHeater);
    RUN_TEST(test_H3_ShortedSensor_ShutoffHeater);
    RUN_TEST(test_M1_SetpointChange_ResetsIntegral);
    return UNITY_END();
}
