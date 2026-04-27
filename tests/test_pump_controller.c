/* =============================================================================
 * test_pump_controller.c — Phase 2 TDD tests for PumpController.c
 *
 * Issues covered:
 *   H1 (Green): Division by zero in slope calculation when time fields are 0.
 *               Fixed by clamping time counts to a minimum of 1.
 *
 * Compile (from project root):
 *   gcc ... PumpController.c unity.c test_pump_controller.c -lm -o test_pc.exe
 * =============================================================================*/
#include "unity.h"
#include "fakes.h"

/* Module under test */
#include "PumpController.h"

/* blEspressoProfile is declared extern in BLEspressoServices.h (included
 * transitively via PumpController.h).  PumpController.c never accesses it
 * directly, but the linker needs a definition for the extern symbol. */
volatile bleSpressoUserdata_struct blEspressoProfile;

/* ---------------------------------------------------------------------------
 * Helpers — prepare a default valid profile struct
 * --------------------------------------------------------------------------- */
static void fill_default_profile(bleSpressoUserdata_struct *p)
{
    p->prof_preInfusePwr  = 35.0f;   /* 35 % pump power */
    p->prof_preInfuseTmr  =  6.0f;   /* 6 s */
    p->prof_InfusePwr     =100.0f;   /* 100 % */
    p->prof_InfuseTmr     =  5.0f;   /* 5 s */
    p->Prof_DeclinePwr    = 80.0f;   /* 80 % */
    p->Prof_DeclineTmr    =  6.0f;   /* 6 s */
}

/* ---------------------------------------------------------------------------
 * setUp / tearDown
 * --------------------------------------------------------------------------- */
void setUp(void)
{
    RESET_ALL_HW_FAKES();
    /* Make solenoid appear engaged immediately so state advances past s00 */
    get_SolenoidSSR_State_fake.return_val = SSR_STATE_ENGAGE;
}

void tearDown(void) {}

/* ---------------------------------------------------------------------------
 * Test 1 — Init returns PUMPCTRL_INIT_OK
 * --------------------------------------------------------------------------- */
void test_PC_Init_Returns_OK(void)
{
    pumpCtrl_status_t result = fcn_initPumpController();
    TEST_ASSERT_EQUAL_INT(PUMPCTRL_INIT_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 2 — After init, state driver returns PUMPCTRL_IDLE (state = s0_Idle)
 * --------------------------------------------------------------------------- */
void test_PC_Init_StateDriver_Returns_IDLE(void)
{
    fcn_initPumpController();
    pumpCtrl_status_t status = fcn_PumpStateDriver();
    TEST_ASSERT_EQUAL_INT(PUMPCTRL_IDLE, (int)status);
}

/* ---------------------------------------------------------------------------
 * Test 3 — Load valid parameters returns PUMPCTRL_LOAD_OK
 * --------------------------------------------------------------------------- */
void test_PC_LoadNewParams_Valid_Returns_OK(void)
{
    bleSpressoUserdata_struct profile;
    fill_default_profile(&profile);

    pumpCtrl_status_t result = fcn_LoadNewPumpParameters(&profile);
    TEST_ASSERT_EQUAL_INT(PUMPCTRL_LOAD_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 4 — H1 (Green): Loading parameters with zero decline time must NOT
 *           crash.  Before fix: integer division by zero → SIGFPE / hard fault.
 *           After fix: function completes, slopes are clamped, returns OK.
 * --------------------------------------------------------------------------- */
void test_H1_ZeroDeclineTime_NoCrash_Returns_OK(void)
{
    bleSpressoUserdata_struct profile;
    fill_default_profile(&profile);
    profile.Prof_DeclineTmr = 0.0f;  /* zero time should not divide by zero */

    /* With the H1 fix this call must not crash and must return LOAD_OK */
    pumpCtrl_status_t result = fcn_LoadNewPumpParameters(&profile);
    TEST_ASSERT_EQUAL_INT(PUMPCTRL_LOAD_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 5 — H1 (extended): all timing fields zero — no crash
 * --------------------------------------------------------------------------- */
void test_H1_AllZeroTimes_NoCrash(void)
{
    bleSpressoUserdata_struct profile;
    fill_default_profile(&profile);
    profile.prof_preInfuseTmr = 0.0f;
    profile.prof_InfuseTmr    = 0.0f;
    profile.Prof_DeclineTmr   = 0.0f;

    pumpCtrl_status_t result = fcn_LoadNewPumpParameters(&profile);
    TEST_ASSERT_EQUAL_INT(PUMPCTRL_LOAD_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 6 — fcn_StartBrew transitions state away from Idle
 *           (solenoid will be activated on the first driver tick)
 * --------------------------------------------------------------------------- */
void test_PC_StartBrew_ThenDriver_CallsSolenoidOn(void)
{
    fcn_initPumpController();

    bleSpressoUserdata_struct profile;
    fill_default_profile(&profile);
    fcn_LoadNewPumpParameters(&profile);

    fcn_StartBrew();          /* set state = s00_CloseSolenoid */
    fcn_PumpStateDriver();    /* s00 → calls fcn_SolenoidSSR_On, checks SSR_STATE_ENGAGE */

    TEST_ASSERT_EQUAL_UINT(1, fcn_SolenoidSSR_On_fake.call_count);
}

/* ---------------------------------------------------------------------------
 * Test 7 — fcn_CancelBrew while ramping causes pump to stop
 * --------------------------------------------------------------------------- */
void test_PC_CancelBrew_StopsPump(void)
{
    fcn_initPumpController();

    bleSpressoUserdata_struct profile;
    fill_default_profile(&profile);
    fcn_LoadNewPumpParameters(&profile);

    /* Start brew and drive a few ticks to get past s00 (solenoid engage) */
    fcn_StartBrew();
    for (int i = 0; i < 5; i++) {
        fcn_PumpStateDriver();
    }

    /* Cancel and keep driving until pump stops */
    fcn_CancelBrew();
    for (int i = 0; i < 50; i++) {
        fcn_PumpStateDriver();
    }

    /* After cancelling and running to completion, pump power must be 0 */
    TEST_ASSERT_EQUAL_UINT16(0,
        fcn_pumpSSR_pwrUpdate_fake.arg0_history[
            fcn_pumpSSR_pwrUpdate_fake.call_count - 1]);
}

/* ---------------------------------------------------------------------------
 * Unity test runner
 * --------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_PC_Init_Returns_OK);
    RUN_TEST(test_PC_Init_StateDriver_Returns_IDLE);
    RUN_TEST(test_PC_LoadNewParams_Valid_Returns_OK);
    RUN_TEST(test_H1_ZeroDeclineTime_NoCrash_Returns_OK);
    RUN_TEST(test_H1_AllZeroTimes_NoCrash);
    RUN_TEST(test_PC_StartBrew_ThenDriver_CallsSolenoidOn);
    RUN_TEST(test_PC_CancelBrew_StopsPump);
    return UNITY_END();
}
