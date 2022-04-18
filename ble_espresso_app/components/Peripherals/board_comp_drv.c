
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "board_comp_drv.h"

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



//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	bsp_event_handler
 * Description: Function for handling events from the BSP module.
 * Caveats:
 * Parameters:  event   Event generated when button is pressed.
 * Return:
 *****************************************************************************/
void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;                                                                                                                                                                                        

    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break; // BSP_EVENT_SLEEP

        case BSP_EVENT_DISCONNECT:
            ble_disconnect();
            break; // BSP_EVENT_DISCONNECT

        case BSP_EVENT_WHITELIST_OFF:
            ble_restart_without_whitelist();
            
            break; // BSP_EVENT_KEY_0

        default:
            break;
    }
}


/*****************************************************************************
 * Function: 	buttons_leds_init
 * Description: Function for initializing buttons and leds.
 * Caveats:
 * Parameters:	 p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 * Return:
 *****************************************************************************/

void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
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