
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "dc12Vouput_drv.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************

//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************


//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
StateMachine12Vout_Struct s12Vout;
volatile uint32_t seqCounter;
volatile uint32_t startFlag;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
static uint16_t /*const*/ seq_ONvalues[] =
    {
        0,
        1004,
        1006,
        1010,
        1015,
        1025,
        1039,
        1062,
        1097,
        1151,
        1229,
        1337,
        1477,
        1644,
        1821,
        1991,
        2137,
        2252,
        2336,
        2393,
        2432,
        2457,
        2473,
        2483,
        2487
    };
    nrf_pwm_sequence_t seqOn  =
    {
        .values.p_common = seq_ONvalues,
        .length          = NRF_PWM_VALUES_LENGTH(seq_ONvalues),
        .repeats         = 0,
        .end_delay       = 0,
    };

    static uint16_t /*const*/ seq_DimmDown[] =
    {
        2487,
        2483,
        2473,
        2457,
        2432,
        2393,
        2336,
        2252
    };
    nrf_pwm_sequence_t seqDimDown  =
    {
        .values.p_common = seq_DimmDown,
        .length          = NRF_PWM_VALUES_LENGTH(seq_DimmDown),
        .repeats         = 0,
        .end_delay       = 0,
    };

    static uint16_t /*const*/ seq_DimmUp[] =
    {
        2252,
        2336,
        2393,
        2432,
        2457,
        2473,
        2483,
        2487
    };
    nrf_pwm_sequence_t seqDimUp  =
    {
        .values.p_common = seq_DimmUp,
        .length          = NRF_PWM_VALUES_LENGTH(seq_DimmUp),
        .repeats         = 0,
        .end_delay       = 0,
    };

    static uint16_t /*const*/ seq_OFFvalues[] =
    {
        2487,
        2483,
        2473,
        2457,
        2432,
        2393,
        2336,
        2252,
        2137,
        1991,
        1821,
        1644,
        1477,
        1337,
        1229,
        1151,
        1097,
        1062,
        1039,
        1025,
        1015,
        1010,
        1006,
        1004,
        100,
        100,
    };
    nrf_pwm_sequence_t seqOff  =
    {
        .values.p_common = seq_OFFvalues,
        .length          = NRF_PWM_VALUES_LENGTH(seq_OFFvalues),
        .repeats         = 0,
        .end_delay       = 0,
    };

    

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************



//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	fcn_initDC12Voutput_drv
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_initDC12Voutput_drv(void)
{
    s12Vout.sDrv.sRunning = st_Idle;
    s12Vout.outState = outst_OFF;
    //Init ouput for 12VDC output
    //------------------------------------------------------------------------
    static nrf_drv_pwm_config_t const config0 =
    {
        .output_pins =
        {
            enDC12Voutput_PIN,                    // channel 0
            NRF_DRV_PWM_PIN_NOT_USED,             // channel 1
            NRF_DRV_PWM_PIN_NOT_USED,             // channel 2
            NRF_DRV_PWM_PIN_NOT_USED,             // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_250kHz,         // 4 us period
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = 2500,
        .load_mode    = NRF_PWM_LOAD_COMMON,
        .step_mode    = NRF_PWM_STEP_TRIGGERED 
    };

    APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm0, &config0, NULL));

    nrf_drv_pwm_simple_playback(&m_pwm0, &seqOn, 1, NRFX_PWM_FLAG_LOOP);
    startFlag = 0;
    seqCounter = 0;
}

/*****************************************************************************
 * Function: 	sm_DC12Voutput_drv
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void sm_DC12Voutput_drv(StateMachine12Vout_Struct *ptr_drvState)
{
  if(startFlag == 1)
  {
      startFlag = 0;
      ptr_drvState->sDrv.sRunning = st_LoadDimOn;
  }
  if(startFlag == 2)
  {
      startFlag = 0;
      ptr_drvState->sDrv.sRunning = st_LoadDimDown;
  }
  if(startFlag == 3)
  {
      startFlag = 0;
      ptr_drvState->sDrv.sRunning = st_LoadDimUp;
  }
  if(startFlag == 4)
  {
      startFlag = 0;
      ptr_drvState->sDrv.sRunning = st_LoadDimOff;
  }

  switch(ptr_drvState->sDrv.sRunning)
  {
    case st_Idle:
      seqCounter = 0;
    break;

    case st_LoadDimOn:
      nrf_drv_pwm_simple_playback(&m_pwm0, &seqOn, 1, NRFX_PWM_FLAG_LOOP);
      ptr_drvState->sDrv.sRunning = st_turningON;
      seqCounter=0;
    break;

    case st_turningON:
      nrf_drv_pwm_step(&m_pwm0);
      seqCounter++;
      if(seqCounter == (seqOn.length-2))
      {
         nrf_drv_pwm_stop(&m_pwm0, false);
         ptr_drvState->sDrv.sRunning = st_Idle;
         ptr_drvState->outState = outst_ON;
      }
    break;

    case st_LoadDimDown:
      nrf_drv_pwm_simple_playback(&m_pwm0, &seqDimDown, 1, NRFX_PWM_FLAG_LOOP);
      ptr_drvState->sDrv.sRunning = st_DimDown;
      seqCounter=0;
    break;

    case st_DimDown:
      nrf_drv_pwm_step(&m_pwm0);
      seqCounter++;
      if(seqCounter == (seqDimDown.length-2))
      {
         ptr_drvState->sDrv.sRunning = st_Idle;
         ptr_drvState->outState = outst_2_3;
      }
    break;

    case st_LoadDimUp:
      nrf_drv_pwm_simple_playback(&m_pwm0, &seqDimUp, 1, NRFX_PWM_FLAG_LOOP);
      ptr_drvState->sDrv.sRunning = st_DimUp;
      seqCounter=0;
    break;

    case st_DimUp:
      nrf_drv_pwm_step(&m_pwm0);
      seqCounter++;
      if(seqCounter == (seqDimUp.length-2))
      {
         nrf_drv_pwm_stop(&m_pwm0, false);
         ptr_drvState->sDrv.sRunning = st_Idle;
         ptr_drvState->outState = outst_ON;
      }
    break;

    case st_LoadDimOff:
      nrf_drv_pwm_simple_playback(&m_pwm0, &seqOff, 1, NRFX_PWM_FLAG_LOOP);
      ptr_drvState->sDrv.sRunning = st_turningOFF;
      seqCounter=0;
    break;

    case st_turningOFF:
      nrf_drv_pwm_step(&m_pwm0);
      seqCounter++;
      if(seqCounter == (seqOff.length-2))
      {
         ptr_drvState->sDrv.sRunning = st_Idle;
         ptr_drvState->outState = outst_OFF;
      }
    break;

    default:

    break;
  }

}

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	InitClocks
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
 // Public function 2 