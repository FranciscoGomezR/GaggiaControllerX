
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


//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
struct_ControllerInputs sControllerInputs;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************


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
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_initACinput_drv(struct_ControllerInputs *ptr_instance)
{
  ret_code_t err_code;
  err_code = nrf_drv_gpiote_init();
  APP_ERROR_CHECK(err_code);
  //inSwitchBrew
  //------------------------------------------------------------------------
  nrf_drv_gpiote_in_config_t in_configBrew = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
  in_configBrew.pull = NRF_GPIO_PIN_NOPULL;
  in_configBrew.sense = GPIOTE_CONFIG_POLARITY_Toggle;
  err_code = nrf_drv_gpiote_in_init(ptr_instance->Brew.pinID,
                                     &in_configBrew, 
                                     ptr_instance->Brew.ext_isr_handler);
  APP_ERROR_CHECK(err_code);
  nrf_drv_gpiote_in_event_enable(ptr_instance->Brew.pinID, true);

  //inSwitchSteam 
  //------------------------------------------------------------------------
  nrf_drv_gpiote_in_config_t in_configSteam = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
  in_configSteam.pull = NRF_GPIO_PIN_NOPULL;
  in_configSteam.sense = GPIOTE_CONFIG_POLARITY_Toggle;
  err_code = nrf_drv_gpiote_in_init(ptr_instance->Steam.pinID,
                                     &in_configSteam, 
                                     ptr_instance->Steam.ext_isr_handler);
  APP_ERROR_CHECK(err_code);
  nrf_drv_gpiote_in_event_enable(ptr_instance->Steam.pinID, true);

}

/*****************************************************************************
 * Function: 	fcn_ACinput_drv
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_ACinput_drv(struct_ControllerInputs *ptr_instance)
{
    fcn_inputLogic(&ptr_instance->Brew);
    fcn_inputLogic(&ptr_instance->Steam);
}


//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	fcn_inputLogic
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_inputLogic(struct_AcInputPin *ptr_instance)
{
    if(ptr_instance->Counter > ptr_instance->nCycles)
    {
        ptr_instance->Status = true;
    }else{
        ptr_instance->Status = false;
    }
    if( ptr_instance->Status != ptr_instance->prevStatus)
    {
        ptr_instance->smEvent = smS_Change;
    }else{
        ptr_instance->smEvent = smS_NoChange;
    }
    ptr_instance->Counter = 0;
    ptr_instance->prevStatus = ptr_instance->Status;
}