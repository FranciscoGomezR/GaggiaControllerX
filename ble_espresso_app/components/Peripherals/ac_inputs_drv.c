
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "ac_inputs_drv.h"

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
typedef struct
{
  uint8_t     pinID;
  uint16_t    Counter;
  uint8_t     Status;
  uint8_t     prevStatus;
  uint8_t     smEvent;
  uint16_t    nCycles;
  nrfx_gpiote_evt_handler_t ext_isr_handler;
}struct_AcInputPin;

typedef struct
{
  struct_AcInputPin  Brew;
  struct_AcInputPin  Steam;
}struct_ControllerInputs;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
static struct_ControllerInputs sIO_ACinput;

//*****************************************************************************
//
//			ISR HANDLERS FUCNTIONS
//
//*****************************************************************************
  /*****************************************************************************
  * Function: 	acinSteam_eventHandler
  * Description: Count number of AC cycle when swicth is active 
  * Definition: ac_inputs_drv.h
  *****************************************************************************/
  void acinSteam_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
  {
      sIO_ACinput.Steam.Counter++;
  }

  /*****************************************************************************
  * Function: 	acinBrew_eventHandler
  * Description: Count number of AC cycle when swicth is active  
  * Definition: ac_inputs_drv.h
  *****************************************************************************/
  void acinBrew_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
  {
      sIO_ACinput.Brew.Counter++;
  }

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************
void fcn_inputLogic(struct_AcInputPin *ptr_instance);


//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	fcn_initACinputs_drv
 * Description: Init the GPIO driver.
                and configure two GPIO as inputs (toggle) &  external interrupts
 * Caveats:     THIS FCN INIT THE GPIO DRIVER
 *****************************************************************************/
acInput_status_t fcn_initACinput_drv(void)
{
  sIO_ACinput.Brew.pinID          = BREW_CFG_PIN_ID;
  sIO_ACinput.Brew.Status         = BREW_CFG_STATUS;
  sIO_ACinput.Brew.smEvent        = AC_INPUT_NO_CHANGE;
  sIO_ACinput.Brew.nCycles        = BREW_CFG_THRESHOLD_N;
  sIO_ACinput.Brew.ext_isr_handler = BREW_CFG_EVT_HANDLER;
  sIO_ACinput.Steam.pinID         = inSTEAM_PIN;
  sIO_ACinput.Steam.Status        = STEAM_CFG_STATUS;
  sIO_ACinput.Steam.smEvent       = AC_INPUT_NO_CHANGE;
  sIO_ACinput.Steam.nCycles       = STEAM_CFG_THRESHOLD_N;
  sIO_ACinput.Steam.ext_isr_handler = STEAM_CFG_EVT_HANDLER;

  ret_code_t err_code;
 
  //inSwitchBrew
  //------------------------------------------------------------------------
  nrf_drv_gpiote_in_config_t in_configBrew = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
  in_configBrew.pull = NRF_GPIO_PIN_NOPULL;
  in_configBrew.sense = GPIOTE_CONFIG_POLARITY_Toggle;
  err_code = nrf_drv_gpiote_in_init(sIO_ACinput.Brew.pinID,
                                     &in_configBrew, 
                                     sIO_ACinput.Brew.ext_isr_handler);
  APP_ERROR_CHECK(err_code);
  nrf_drv_gpiote_in_event_enable(sIO_ACinput.Brew.pinID, true);

  //inSwitchSteam 
  //------------------------------------------------------------------------
  nrf_drv_gpiote_in_config_t in_configSteam = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
  in_configSteam.pull = NRF_GPIO_PIN_NOPULL;
  in_configSteam.sense = GPIOTE_CONFIG_POLARITY_Toggle;
  err_code = nrf_drv_gpiote_in_init(sIO_ACinput.Steam.pinID,
                                     &in_configSteam, 
                                     sIO_ACinput.Steam.ext_isr_handler);
  APP_ERROR_CHECK(err_code);
  nrf_drv_gpiote_in_event_enable(sIO_ACinput.Steam.pinID, true);

  return DRV_AC_INPUT_INIT_OK;
}

/*****************************************************************************
 * Function: 	fcn_ACinput_drv
 * Description: Drivers the logic behind the detection of both switch activation
                by detecting no. of cycle above the threshold no.
 *****************************************************************************/
void fcn_ACinput_drv(void)
{
    fcn_inputLogic(&sIO_ACinput.Brew);
    fcn_inputLogic(&sIO_ACinput.Steam);
}

/*****************************************************************************
 * Function: 	fcn_GetInputStatus_Brew
 * Description: this fcn retrieves the status parameter of BREW pin
 * Return:      TRUE if drv detecte a higher no. of cycle than the threshold lvl
                FALSE if there is no cycle detected or no. is below threshold
 *****************************************************************************/
acInput_status_t fcn_GetInputStatus_Brew(void)
{
  return sIO_ACinput.Brew.Status;
}

/*****************************************************************************
 * Function: 	fcn_GetInputStatus_Steam
 * Description: this fcn retrieves the status parameter of STEAM pin
 * Return:      TRUE if drv detecte a higher no. of cycle than the threshold lvl
                FALSE if there is no cycle detected or no. is below threshold
 *****************************************************************************/
acInput_status_t fcn_GetInputStatus_Steam(void)
{
  return sIO_ACinput.Steam.Status;
}

/*****************************************************************************
 * Function: 	fcn_StatusChange_Brew
 * Description: this fcn asks the drv if an event (Off->on or Off->on) was detected
 * Return:      0 -> No change in the status of the pin
                1 -> status of the pin changed 
 *****************************************************************************/
acInput_status_t fcn_StatusChange_Brew(void)
{
  return sIO_ACinput.Brew.smEvent;
}

/*****************************************************************************
 * Function: 	fcn_StatusChange_Steam
 * Description: this fcn asks the drv if an event (Off->on or Off->on) was detected
 * Return:      0 -> No change in the status of the pin
                1 -> status of the pin changed 
 *****************************************************************************/
acInput_status_t fcn_StatusChange_Steam(void)
{
  return sIO_ACinput.Steam.smEvent;
}

/*****************************************************************************
 * Function: 	fcn_StatusReset_Brew
 * Description: Reset the following field: smEvent to: AC_INPUT_NO_CHANGE
 *****************************************************************************/
void fcn_StatusReset_Brew(void)
{
  sIO_ACinput.Brew.smEvent = AC_INPUT_NO_CHANGE;
}

/*****************************************************************************
 * Function: 	fcn_StatusReset_Steam
 * Description: Reset the following field: smEvent to: AC_INPUT_NO_CHANGE
 *****************************************************************************/
void fcn_StatusReset_Steam(void)
{
  sIO_ACinput.Steam.smEvent = AC_INPUT_NO_CHANGE;
}

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	fcn_inputLogic
 * Description: 
 *****************************************************************************/
void fcn_inputLogic(struct_AcInputPin *ptr_instance)
{
    if(ptr_instance->Counter > ptr_instance->nCycles)
    {
        //number of AC zero-crossing events are more than nCyles, 
        //it means the switch is close
        ptr_instance->Status = (uint8_t)AC_INPUT_ASSERTED;
    }else{
        //number of AC zero-crossing events are less than nCyles or even 0 cyles,
        //it means the switch is OPEN
        ptr_instance->Status = (uint8_t)AC_INPUT_DEASSERTED;
    }
    //IF STATEMENT to detect changes events, from Close-To-Open or from Open-To-Close
    if( ptr_instance->Status != ptr_instance->prevStatus)
    {
        //Event change detected, update
        ptr_instance->smEvent = AC_INPUT_CHANGE;
    }else{
        //Event change not detected
        ptr_instance->smEvent = AC_INPUT_NO_CHANGE;
    }
    //Cycle counter reset and saved current status to be evaluted in the next iteration
    ptr_instance->Counter = 0;
    ptr_instance->prevStatus = ptr_instance->Status;
}