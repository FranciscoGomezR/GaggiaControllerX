/* =============================================================================
 * test_storage_controller.c — Phase 2 TDD tests for StorageController.c
 *
 * Issues covered:
 *   M3 (documented): No NVM data range validation — current behavior captured.
 *
 * Strategy:
 *   spi_NVMemoryRead and spi_NVMemoryWritePage are replaced by FFF custom-fake
 *   callbacks that read from / write to a local g_nvm_page[65] buffer.
 *   The three private helpers (parsingBytesToFloat, parsingBytesTo32bitVar,
 *   encodeFloatToBytes) are forward-declared and tested directly because they
 *   are not static.
 *
 * Compile (from project root):
 *   gcc ... StorageController.c unity.c test_storage_controller.c -lm
 * =============================================================================*/
#include "unity.h"
#include "fakes.h"

/* Forward-declare StorageController's non-static private helpers so we can
 * call them directly from tests */
void parsingBytesToFloat(uint8_t *ptr_Fbytes, float *ptr_Fnumber);
void parsingBytesTo32bitVar(uint8_t *ptr_Fbytes, uint32_t *ptr_number);
void encodeFloatToBytes(float fnumber, uint8_t *ptr_Fbytes);

/* Module under test */
#include "StorageController.h"

/* blEspressoProfile is declared extern (via BLEspressoServices.h chain).
 * StorageController.c does NOT access it globally, but the extern symbol
 * must have a definition to satisfy the linker. */
volatile bleSpressoUserdata_struct blEspressoProfile;

/* ---------------------------------------------------------------------------
 * NVM page simulation buffer
 * --------------------------------------------------------------------------- */
#define NVM_PAGE_SIZE   65

static uint8_t g_nvm_page[NVM_PAGE_SIZE];
static uint8_t g_written_buf[NVM_PAGE_SIZE];

/* FFF custom-fake callbacks ------------------------------------------------- */
static void fake_NVMRead(uint32_t pg, uint8_t off, uint32_t n, uint8_t *buf)
{
    (void)pg;
    if (off + n > NVM_PAGE_SIZE) n = NVM_PAGE_SIZE - off;
    memcpy(buf, g_nvm_page + off, n);
}

static void fake_NVMWrite(uint32_t pg, uint8_t off, uint32_t n, uint8_t *buf)
{
    (void)pg; (void)off;
    if (n > NVM_PAGE_SIZE) n = NVM_PAGE_SIZE;
    memcpy(g_written_buf, buf, n);
}

/* ---------------------------------------------------------------------------
 * NVM helpers — encode/decode the fixed byte layout expected by
 * StorageController.c (see address map in StorageController.c).
 *
 * Offsets (from BE_USERDATA_* defines in StorageController.c):
 *   0x04  uint32   NVM key
 *   0x08  float    temp_Target
 *   0x10  float    prof_preInfusePwr
 *   0x14  float    prof_preInfuseTmr
 *   0x18  float    prof_InfusePwr
 *   0x1C  float    prof_InfuseTmr
 *   0x20  float    Prof_DeclinePwr
 *   0x24  float    Prof_DeclineTmr
 *   0x28  float    Pid_P_term
 *   0x2C  float    Pid_I_term
 *   0x30  float    Pid_Imax_term
 *   0x34  float    Pid_D_term
 *   0x38  float    Pid_Dlpf_term
 *   0x3C  float    Pid_Gain_term
 *   0x40  bool     Pid_Iwindup_term
 * --------------------------------------------------------------------------- */
#define NVM_KEY_VALID    0x00AA00AA
#define NVM_KEY_EMPTY    0xFFFFFFFF

static void nvm_write_u32(uint8_t *page, uint8_t off, uint32_t val)
{
    page[off+0] = (uint8_t)( val        & 0xFF);
    page[off+1] = (uint8_t)((val >>  8) & 0xFF);
    page[off+2] = (uint8_t)((val >> 16) & 0xFF);
    page[off+3] = (uint8_t)((val >> 24) & 0xFF);
}

static void nvm_write_float(uint8_t *page, uint8_t off, float val)
{
    uint32_t bits;
    memcpy(&bits, &val, 4);
    page[off+0] = (uint8_t)( bits        & 0xFF);
    page[off+1] = (uint8_t)((bits >>  8) & 0xFF);
    page[off+2] = (uint8_t)((bits >> 16) & 0xFF);
    page[off+3] = (uint8_t)((bits >> 24) & 0xFF);
}

static float nvm_read_float(const uint8_t *page, uint8_t off)
{
    uint32_t bits = ((uint32_t)page[off+0]       ) |
                    ((uint32_t)page[off+1] <<  8 ) |
                    ((uint32_t)page[off+2] << 16 ) |
                    ((uint32_t)page[off+3] << 24 );
    float val; memcpy(&val, &bits, 4);
    return val;
}

static void setup_valid_nvm_page(float temp_target, float pid_p)
{
    memset(g_nvm_page, 0x00, NVM_PAGE_SIZE);
    nvm_write_u32  (g_nvm_page, 0x04, NVM_KEY_VALID);
    nvm_write_float(g_nvm_page, 0x08, temp_target);
    nvm_write_float(g_nvm_page, 0x28, pid_p);          /* Pid_P_term */
    nvm_write_float(g_nvm_page, 0x2C, 0.3f);           /* Pid_I_term */
    nvm_write_float(g_nvm_page, 0x30, 100.0f);         /* Pid_Imax */
}

/* ---------------------------------------------------------------------------
 * setUp / tearDown
 * --------------------------------------------------------------------------- */
void setUp(void)
{
    RESET_ALL_HW_FAKES();
    memset(g_nvm_page,    0xFF, NVM_PAGE_SIZE);
    memset(g_written_buf, 0x00, NVM_PAGE_SIZE);

    spi_NVMemoryRead_fake.custom_fake      = fake_NVMRead;
    spi_NVMemoryWritePage_fake.custom_fake = fake_NVMWrite;
    spim_initNVmemory_fake.return_val      = NVM_INIT_OK;
}

void tearDown(void) {}

/* ---------------------------------------------------------------------------
 * Test 1 — stgCtrl_Init returns the value from spim_initNVmemory
 * --------------------------------------------------------------------------- */
void test_SC_Init_Returns_NVM_INIT_OK(void)
{
    uint32_t result = stgCtrl_Init();
    TEST_ASSERT_EQUAL_UINT32(NVM_INIT_OK, result);
}

/* ---------------------------------------------------------------------------
 * Test 2 — stgCtrl_ChkForUserData: valid magic key → STORAGE_USERDATA_LOADED
 * --------------------------------------------------------------------------- */
void test_SC_ChkForUserData_ValidKey_Returns_LOADED(void)
{
    nvm_write_u32(g_nvm_page, 0x04, NVM_KEY_VALID);
    uint32_t result = stgCtrl_ChkForUserData();
    TEST_ASSERT_EQUAL_UINT32(STORAGE_USERDATA_LOADED, result);
}

/* ---------------------------------------------------------------------------
 * Test 3 — stgCtrl_ChkForUserData: erased (0xFF…) page → STORAGE_USERDATA_EMPTY
 * --------------------------------------------------------------------------- */
void test_SC_ChkForUserData_EmptyKey_Returns_EMPTY(void)
{
    /* g_nvm_page already filled with 0xFF in setUp */
    uint32_t result = stgCtrl_ChkForUserData();
    TEST_ASSERT_EQUAL_UINT32(STORAGE_USERDATA_EMPTY, result);
}

/* ---------------------------------------------------------------------------
 * Test 4 — stgCtrl_ReadUserData: encode known floats → struct fields match
 * --------------------------------------------------------------------------- */
void test_SC_ReadUserData_ValidKey_Deserialises_Correctly(void)
{
    setup_valid_nvm_page(93.0f, 9.52156f);

    bleSpressoUserdata_struct rx;
    memset(&rx, 0, sizeof(rx));

    uint32_t result = stgCtrl_ReadUserData(&rx);

    TEST_ASSERT_EQUAL_UINT32(STORAGE_USERDATA_LOADED, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f,    rx.temp_Target);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.52156f, rx.Pid_P_term);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.3f,     rx.Pid_I_term);
}

/* ---------------------------------------------------------------------------
 * Test 5 — stgCtrl_ReadUserData: invalid key → STORAGE_USERDATA_EMPTY,
 *           struct unchanged
 * --------------------------------------------------------------------------- */
void test_SC_ReadUserData_InvalidKey_Returns_EMPTY(void)
{
    /* Key is 0xDEADBEEF — neither valid nor empty */
    nvm_write_u32(g_nvm_page, 0x04, 0xDEADBEEFUL);

    bleSpressoUserdata_struct rx;
    memset(&rx, 0xAB, sizeof(rx));   /* fill with sentinel value */

    uint32_t result = stgCtrl_ReadUserData(&rx);
    TEST_ASSERT_EQUAL_UINT32(STORAGE_USERDATA_EMPTY, result);
}

/* ---------------------------------------------------------------------------
 * Test 6 — M3 (Green): stgCtrl_ReadUserData with an out-of-range temp_Target
 *           value (999 °C) now clamps it to the safe default (93.0 °C).
 *           This test was previously named test_M3_OutOfRange_Temp_Loaded_Without_Validation
 *           and documented the broken behaviour.  After the M3 fix the expected
 *           value changes from 999.0 to 93.0.
 * --------------------------------------------------------------------------- */
void test_M3_OutOfRange_Temp_Clamped_To_Default(void)
{
    setup_valid_nvm_page(999.0f, 9.52156f);   /* absurd temperature */

    bleSpressoUserdata_struct rx;
    memset(&rx, 0, sizeof(rx));

    uint32_t result = stgCtrl_ReadUserData(&rx);
    TEST_ASSERT_EQUAL_UINT32(STORAGE_USERDATA_LOADED, result);
    /* M3 fix: 999 °C is outside [20, 110] → clamped to default 93.0 °C */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f, rx.temp_Target);
}

/* ---------------------------------------------------------------------------
 * Test 6b — M3: NaN bytes in a PID field → clamped to default
 * --------------------------------------------------------------------------- */
void test_M3_NaN_PidPterm_Clamped(void)
{
    setup_valid_nvm_page(93.0f, 9.5f);
    nvm_write_float(g_nvm_page, 0x28, (float)NAN);   /* Pid_P_term = NaN */

    bleSpressoUserdata_struct rx;
    memset(&rx, 0, sizeof(rx));

    stgCtrl_ReadUserData(&rx);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.5f, rx.Pid_P_term);  /* clamped to default */
}

/* ---------------------------------------------------------------------------
 * Test 6c — M3: negative timer value in NVM → clamped to default
 * --------------------------------------------------------------------------- */
void test_M3_NegativeTimer_Clamped(void)
{
    setup_valid_nvm_page(93.0f, 9.5f);
    nvm_write_float(g_nvm_page, 0x14, -5.0f);   /* prof_preInfuseTmr = -5.0 */

    bleSpressoUserdata_struct rx;
    memset(&rx, 0, sizeof(rx));

    stgCtrl_ReadUserData(&rx);
    /* -5.0 is below [0, 15] → clamped to default 3.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, rx.prof_preInfuseTmr);
}

/* ---------------------------------------------------------------------------
 * Test 6d — M3 regression: all fields in valid range → no change
 * --------------------------------------------------------------------------- */
void test_M3_AllFieldsValid_NoChange(void)
{
    setup_valid_nvm_page(93.0f, 9.5f);
    /* Add other sensible values */
    nvm_write_float(g_nvm_page, 0x14,  3.0f);   /* prof_preInfuseTmr */
    nvm_write_float(g_nvm_page, 0x18, 100.0f);  /* prof_InfusePwr */
    nvm_write_float(g_nvm_page, 0x1C, 25.0f);   /* prof_InfuseTmr */

    bleSpressoUserdata_struct rx;
    memset(&rx, 0, sizeof(rx));

    uint32_t result = stgCtrl_ReadUserData(&rx);
    TEST_ASSERT_EQUAL_UINT32(STORAGE_USERDATA_LOADED, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f, rx.temp_Target);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  9.5f, rx.Pid_P_term);
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  3.0f, rx.prof_preInfuseTmr);
}

/* ---------------------------------------------------------------------------
 * Test 7 — stgCtrl_StoreShotProfileData: first write (empty NVM) encodes key
 * --------------------------------------------------------------------------- */
void test_SC_StoreShotProfile_FirstWrite_SetsKey(void)
{
    /* NVM is empty (0xFF) — first write path */
    bleSpressoUserdata_struct tx;
    memset(&tx, 0, sizeof(tx));
    tx.temp_Target      = 93.0f;
    tx.prof_preInfusePwr= 35.0f;
    tx.prof_preInfuseTmr=  6.0f;
    tx.prof_InfusePwr   =100.0f;
    tx.prof_InfuseTmr   =  5.0f;
    tx.Prof_DeclinePwr  = 80.0f;
    tx.Prof_DeclineTmr  =  6.0f;

    stgCtrl_StoreShotProfileData(&tx);

    /* Key bytes at offset 4 must be 0xAA, 0x00, 0xAA, 0x00 */
    TEST_ASSERT_EQUAL_UINT8(0xAA, g_written_buf[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_written_buf[5]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, g_written_buf[6]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_written_buf[7]);

    /* temp_Target must round-trip through IEEE 754 encoding */
    float decoded_temp = nvm_read_float(g_written_buf, 0x08);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f, decoded_temp);
}

/* ---------------------------------------------------------------------------
 * Test 8 — stgCtrl_StoreControllerData: shot write-cycle counter increments
 * --------------------------------------------------------------------------- */
void test_SC_StoreController_WriteCycle_Incremented(void)
{
    /* Prepare NVM with existing data and wCycleShotprofile = 5 */
    setup_valid_nvm_page(93.0f, 9.52156f);
    /* Encode wCycles: shot=5 (high 16b), ctrl=3 (low 16b) in little-endian */
    uint32_t wCycles = (5u << 16) | 3u;
    nvm_write_u32(g_nvm_page, 0x00, wCycles);

    bleSpressoUserdata_struct tx;
    memset(&tx, 0, sizeof(tx));
    tx.Pid_P_term      = 9.52156f;
    tx.Pid_I_term      = 0.3f;
    tx.Pid_Imax_term   = 100.0f;
    tx.Pid_D_term      = 0.0f;
    tx.Pid_Dlpf_term   = 0.0f;
    tx.Pid_Gain_term   = 1.0f;
    tx.Pid_Iwindup_term= false;

    stgCtrl_StoreControllerData(&tx);

    /* Read back wCycles: ctrl counter (low 16b in byte 0-1) must be 4 */
    uint16_t ctrl_cycles = ((uint16_t)g_written_buf[1] << 8) |
                            (uint16_t)g_written_buf[0];
    TEST_ASSERT_EQUAL_UINT16(4u, ctrl_cycles);

    /* Shot counter (high 16b, bytes 2-3) must remain 5 */
    uint16_t shot_cycles = ((uint16_t)g_written_buf[3] << 8) |
                            (uint16_t)g_written_buf[2];
    TEST_ASSERT_EQUAL_UINT16(5u, shot_cycles);
}

/* ---------------------------------------------------------------------------
 * Test 9 — parsingBytesToFloat / encodeFloatToBytes round-trip
 * --------------------------------------------------------------------------- */
void test_SC_Private_ParseFloat_RoundTrip(void)
{
    float original = 93.5f;
    uint8_t bytes[4] = {0};

    encodeFloatToBytes(original, bytes);

    float decoded = 0.0f;
    parsingBytesToFloat(bytes, &decoded);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, original, decoded);
}

/* ---------------------------------------------------------------------------
 * Test 10 — encodeFloatToBytes produces expected IEEE 754 LE byte pattern
 *            for the value 1.0f (0x3F800000 LE: 00 00 80 3F)
 * --------------------------------------------------------------------------- */
void test_SC_Private_EncodeFloat_KnownPattern(void)
{
    uint8_t bytes[4] = {0};
    encodeFloatToBytes(1.0f, bytes);

    TEST_ASSERT_EQUAL_UINT8(0x00, bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(0x80, bytes[2]);
    TEST_ASSERT_EQUAL_UINT8(0x3F, bytes[3]);
}

/* ---------------------------------------------------------------------------
 * Unity test runner
 * --------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_SC_Init_Returns_NVM_INIT_OK);
    RUN_TEST(test_SC_ChkForUserData_ValidKey_Returns_LOADED);
    RUN_TEST(test_SC_ChkForUserData_EmptyKey_Returns_EMPTY);
    RUN_TEST(test_SC_ReadUserData_ValidKey_Deserialises_Correctly);
    RUN_TEST(test_SC_ReadUserData_InvalidKey_Returns_EMPTY);
    RUN_TEST(test_M3_OutOfRange_Temp_Clamped_To_Default);
    RUN_TEST(test_M3_NaN_PidPterm_Clamped);
    RUN_TEST(test_M3_NegativeTimer_Clamped);
    RUN_TEST(test_M3_AllFieldsValid_NoChange);
    RUN_TEST(test_SC_StoreShotProfile_FirstWrite_SetsKey);
    RUN_TEST(test_SC_StoreController_WriteCycle_Incremented);
    RUN_TEST(test_SC_Private_ParseFloat_RoundTrip);
    RUN_TEST(test_SC_Private_EncodeFloat_KnownPattern);
    return UNITY_END();
}
