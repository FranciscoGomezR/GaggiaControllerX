
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "ac_inputs_drv.h"
#include "x01_StateMachineControls.h"

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
  uint8_t           pinID;
  volatile uint32_t isr_Counter;
  volatile bool     logicEvaluation;
  uint32_t          counterTop;
  uint8_t           Status;
  nrfx_gpiote_evt_handler_t ext_isr_handler;
}struct_AcInputPin;

typedef struct
{
  struct_AcInputPin  Brew;
  struct_AcInputPin  Steam;
}struct_ControllerInputs;

enum {
  oddOStick = 0,
  evenOStick
};

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
    sIO_ACinput.Steam.isr_Counter++;
    sIO_ACinput.Steam.logicEvaluation = true;
}

/*****************************************************************************
* Function: 	acinBrew_eventHandler
* Description: Count number of AC cycle when swicth is active  
* Definition: ac_inputs_drv.h
*****************************************************************************/
void acinBrew_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    sIO_ACinput.Brew.isr_Counter++;
    sIO_ACinput.Brew.logicEvaluation = true;
}

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************
static void fcn_acInputLogic(struct_AcInputPin* ptr_input);

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
  //inSwitchBrew
  sIO_ACinput.Brew.pinID          = BREW_CFG_PIN_ID;
  sIO_ACinput.Brew.Status         = BREW_CFG_STATUS;
  sIO_ACinput.Brew.ext_isr_handler = BREW_CFG_EVT_HANDLER;
  sIO_ACinput.Brew.logicEvaluation = false;
  //inSwitchSteam 
  sIO_ACinput.Steam.pinID         = inSTEAM_PIN;
  sIO_ACinput.Steam.Status        = STEAM_CFG_STATUS;
  sIO_ACinput.Steam.ext_isr_handler = STEAM_CFG_EVT_HANDLER;
  sIO_ACinput.Steam.logicEvaluation = false;

  ret_code_t err_code;
  //inSwitchBrew
  //------------------------------------------------------------------------
  nrf_drv_gpiote_in_config_t in_configBrew = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
  /*#define NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(hi_accu) \
  {                                                   \
    .sense = NRF_GPIOTE_POLARITY_TOGGLE,            \
    .pull = NRF_GPIO_PIN_NOPULL,                    \
    .is_watcher = false,                            \
    .hi_accuracy = hi_accu,                         \
    .skip_gpio_setup = false,                       \
  }*/
  err_code = nrf_drv_gpiote_in_init(sIO_ACinput.Brew.pinID,
                                     &in_configBrew, 
                                     sIO_ACinput.Brew.ext_isr_handler);
  APP_ERROR_CHECK(err_code);
  nrf_drv_gpiote_in_event_enable(sIO_ACinput.Brew.pinID, true);

  //inSwitchSteam 
  //------------------------------------------------------------------------
  nrf_drv_gpiote_in_config_t in_configSteam = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
  err_code = nrf_drv_gpiote_in_init(sIO_ACinput.Steam.pinID,
                                     &in_configSteam, 
                                     sIO_ACinput.Steam.ext_isr_handler);
  APP_ERROR_CHECK(err_code);
  nrf_drv_gpiote_in_event_enable(sIO_ACinput.Steam.pinID, true);

  return DRV_AC_INPUT_INIT_OK;
}

/*****************************************************************************
 * Function: 	fcn_SenseACinputs_Sixty_ms
 * Description: Drivers the logic behind the detection of both AC inputs.
 *              It shall be called every 60ms to detect proper status change.
 *****************************************************************************/
void fcn_SenseACinputs_Sixty_ms(void)
{
  fcn_acInputLogic(&sIO_ACinput.Brew);
  fcn_acInputLogic(&sIO_ACinput.Steam);
}

/*****************************************************************************
 * Function: 	fcn_GetInputStatus_Brew
 * Description: this fcn retrieves the status parameter of BREW pin
 * Return:      TRUE if drv detecte a higher no. of cycle than the threshold lvl
                FALSE if there is no cycle detected or no. is below threshold
 *****************************************************************************/
acInput_status_t fcn_GetInputStatus_Brew(void)
{
  if(sIO_ACinput.Brew.logicEvaluation == true)
  {
    fcn_acInputLogic(&sIO_ACinput.Brew);
  }else{}
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
  if(sIO_ACinput.Steam.logicEvaluation == true)
  {
    fcn_acInputLogic(&sIO_ACinput.Steam);
  }else{}
  return sIO_ACinput.Steam.Status;
}

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************
static void fcn_acInputLogic(struct_AcInputPin* ptr_input)
{
  if(ptr_input->isr_Counter > ptr_input->counterTop)
  {
    //number of AC zero-crossing events are more than nCyles, 
    //it means the AC-switch is close
    ptr_input->Status = (uint8_t)AC_SWITCH_ASSERTED;
  }else{
    //number of AC zero-crossing events are less than nCyles or even 0 cyles,
    //it means the AC-switch is OPEN
    ptr_input->Status = (uint8_t)AC_SWITCH_DEASSERTED;
  }
  //Saved current values
  ptr_input->counterTop = ptr_input->isr_Counter;
  //Input counter has been evaluated
  ptr_input->logicEvaluation=false;
}
