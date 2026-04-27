/* =============================================================================
 * test_PumpController.c — Phase 3 CMock version of Phase 2 pump controller tests
 *
 * Issues covered:
 *   H1 (Green): Division by zero in slope calculation when time fields are 0.
 *
 * CMock vs FFF (Phase 2) differences demonstrated here:
 *   - "mock_solidStateRelay_Controller.h" is AUTO-GENERATED from the real header;
 *     no manual FAKE_VOID_FUNC() declarations needed.
 *   - Unexpected calls immediately fail the test (enforce_strict_ordering: TRUE).
 *   - setUp() uses _IgnoreAndReturn() / _Ignore() instead of RESET_ALL_HW_FAKES().
 *   - Test 6 uses CMock's strict _Expect() to verify exactly one SolenoidSSR_On call.
 *   - Test 7 uses CMock's _StubWithCallback() to capture the final pump power arg.
 *
 * Run (from tests_ceedling/ directory):
 *   ceedling test:test_PumpController
 * =============================================================================*/
#include "unity.h"
#include "PumpController.h"

/* CMock auto-generates this mock from solidStateRelay_Controller.h.
 * It provides _Expect / _Ignore / _IgnoreAndReturn / _StubWithCallback() APIs. */
#include "mock_solidStateRelay_Controller.h"

/* blEspressoProfile is extern-declared in BLEspressoServices.h (included
 * transitively via PumpController.h).  PumpController.c never accesses the
 * global directly, but the linker still needs a definition. */
volatile bleSpressoUserdata_struct blEspressoProfile;

/* ---------------------------------------------------------------------------
 * Helper — build a default valid profile
 * --------------------------------------------------------------------------- */
static void fill_default_profile(bleSpressoUserdata_struct *p)
{
    p->prof_preInfusePwr  = 35.0f;
    p->prof_preInfuseTmr  =  6.0f;
    p->prof_InfusePwr     =100.0f;
    p->prof_InfuseTmr     =  5.0f;
    p->Prof_DeclinePwr    = 80.0f;
    p->Prof_DeclineTmr    =  6.0f;
}

/* ---------------------------------------------------------------------------
 * setUp / tearDown
 *
 * CMock resets all mock state between tests automatically (via tearDown).
 * setUp provides the default IgnoreAndReturn for functions used by most tests.
 * Individual tests override this for specific assertions.
 * --------------------------------------------------------------------------- */
void setUp(void)
{
    /* All tests need the solenoid state to return ENGAGE so the pump state
     * machine advances past s00_CloseSolenoid in one driver tick. */
    get_SolenoidSSR_State_IgnoreAndReturn(SSR_STATE_ENGAGE);
}

void tearDown(void)
{
    /* CMock automatically verifies all pending Expect() calls here.
     * Any unmet expectation causes an immediate test failure. */
}

/* ---------------------------------------------------------------------------
 * Test 1 — fcn_initPumpController returns PUMPCTRL_INIT_OK.
 *           Init does NOT call any SSR function directly.
 * --------------------------------------------------------------------------- */
void test_PC_Init_Returns_OK(void)
{
    pumpCtrl_status_t result = fcn_initPumpController();
    TEST_ASSERT_EQUAL_INT(PUMPCTRL_INIT_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 2 — After init, PumpStateDriver returns PUMPCTRL_IDLE (state = s0_Idle).
 *           s0_Idle does not call any SSR function.
 * --------------------------------------------------------------------------- */
void test_PC_Init_StateDriver_Returns_IDLE(void)
{
    fcn_initPumpController();
    pumpCtrl_status_t status = fcn_PumpStateDriver();
    TEST_ASSERT_EQUAL_INT(PUMPCTRL_IDLE, (int)status);
}

/* ---------------------------------------------------------------------------
 * Test 3 — fcn_LoadNewPumpParameters with valid profile returns PUMPCTRL_LOAD_OK.
 * --------------------------------------------------------------------------- */
void test_PC_LoadNewParams_Valid_Returns_OK(void)
{
    bleSpressoUserdata_struct profile;
    fill_default_profile(&profile);
    pumpCtrl_status_t result = fcn_LoadNewPumpParameters(&profile);
    TEST_ASSERT_EQUAL_INT(PUMPCTRL_LOAD_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 4 — H1 (Green): zero decline time must NOT crash or divide by zero.
 * --------------------------------------------------------------------------- */
void test_H1_ZeroDeclineTime_NoCrash_Returns_OK(void)
{
    bleSpressoUserdata_struct profile;
    fill_default_profile(&profile);
    profile.Prof_DeclineTmr = 0.0f;

    pumpCtrl_status_t result = fcn_LoadNewPumpParameters(&profile);
    TEST_ASSERT_EQUAL_INT(PUMPCTRL_LOAD_OK, (int)result);
}

/* ---------------------------------------------------------------------------
 * Test 5 — H1 extended: all timing fields zero — no crash
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
 * Test 6 — fcn_StartBrew then one PumpStateDriver tick calls SolenoidSSR_On.
 *
 * CMock strict-ordering demonstration:
 *   fcn_SolenoidSSR_On_Expect() declares EXACTLY ONE call is expected.
 *   If the code calls it 0 or 2+ times, CMock fails the test immediately.
 *   Unexpected calls to ANY other non-Ignored mock also fail instant.
 * --------------------------------------------------------------------------- */
void test_PC_StartBrew_ThenDriver_CallsSolenoidOn(void)
{
    fcn_initPumpController();

    bleSpressoUserdata_struct profile;
    fill_default_profile(&profile);
    fcn_LoadNewPumpParameters(&profile);

    fcn_StartBrew();    /* sets state → s00_CloseSolenoid */

    /* CMock: declare the strict expectation BEFORE the state-machine runs */
    fcn_SolenoidSSR_On_Expect();

    /* Driver executes s00: calls fcn_SolenoidSSR_On() exactly once,
     * then calls get_SolenoidSSR_State() (already covered by setUp _Ignore).
     * tearDown() will verify the Expect was met. */
    fcn_PumpStateDriver();
}

/* ---------------------------------------------------------------------------
 * Test 7 — fcn_CancelBrew while ramping → pump must reach power 0.
 *
 * CMock _StubWithCallback() demonstration:
 *   Replaces the mock for fcn_pumpSSR_pwrUpdate with our callback.
 *   The callback captures every power update; we assert the final one is 0.
 * --------------------------------------------------------------------------- */
static uint16_t s_last_pump_pwr = 0xFFFF;
static void pump_pwr_cb(uint16_t pwr, int num_calls)
{
    (void)num_calls;
    s_last_pump_pwr = pwr;
}

void test_PC_CancelBrew_StopsPump(void)
{
    s_last_pump_pwr = 0xFFFF;

    /* Capture every pump power update via callback; ignore solenoid calls */
    fcn_pumpSSR_pwrUpdate_StubWithCallback(pump_pwr_cb);
    fcn_SolenoidSSR_On_Ignore();
    fcn_SolenoidSSR_Off_Ignore();

    fcn_initPumpController();

    bleSpressoUserdata_struct profile;
    fill_default_profile(&profile);
    fcn_LoadNewPumpParameters(&profile);

    /* Start brew and run several ticks to get past solenoid-engage */
    fcn_StartBrew();
    for (int i = 0; i < 5; i++) {
        fcn_PumpStateDriver();
    }

    /* Cancel and run until the state machine reaches st_Stop */
    fcn_CancelBrew();
    for (int i = 0; i < 50; i++) {
        fcn_PumpStateDriver();
    }

    /* After cancel + run-out, the last pump power must be 0 */
    TEST_ASSERT_EQUAL_UINT16(0, s_last_pump_pwr);
}
