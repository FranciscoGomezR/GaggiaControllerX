#include "sdk_common.h"
#include "ble_cus.h"
#include <string.h>
#include "ble_srv_common.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"
#include "x04_Numbers.h"

//https://www.youtube.com/watch?v=xQwX3yEcAEk&t=8961s&ab_channel=nrf5dev

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

    ble_cus_evt_t evt;

    evt.evt_type = BLE_CUS_EVT_CONNECTED;

    p_cus->evt_handler(p_cus, &evt);
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;
    
    ble_cus_evt_t evt;

    evt.evt_type = BLE_CUS_EVT_DISCONNECTED;

    p_cus->evt_handler(p_cus, &evt);
}


 /*****************************************************************************
 * Function: 	on_write
 * Description: Function for handling Write events.
 * Caveats:     Youtube-TimeStamp: 42:00
 * Parameters:	@param[in]   p_cus       Custom Service structure.
 *              @param[in]   p_ble_evt   Event received from the BLE stack
 * Return:
 *****************************************************************************/
static void on_write(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    ble_cus_evt_t                 evt;
  
    // Check if the Custom value CCCD is written to 
    if ( (p_evt_write->handle == p_cus->blespressoStatus_char_handles.cccd_handle)  )
    {
        // CCCD written, call application event handler
        if (p_cus->evt_handler != NULL)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLESPRESSO_STATUS_CHAR_NOTIFICATION_ENABLED;
            }else{
                evt.evt_type = BLESPRESSO_STATUS_CHAR_NOTIFICATION_DISABLED;
            }
            // Call the application event handler.
            p_cus->evt_handler(p_cus, &evt);
        }
    }
    // Check if the Custom value CCCD is written to and that the value is the appropriate length, i.e 2 bytes.
    //else if ( (p_evt_write->handle == p_cus->boilerTemp_char_handles.cccd_handle) && (p_evt_write->len == 2)  )
    else if ( (p_evt_write->handle == p_cus->boilerTemp_char_handles.cccd_handle)  )
    {
        // CCCD written, call application event handler
        if (p_cus->evt_handler != NULL)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_BOILER_TEMP_CHAR_NOTIFICATION_ENABLED;
            }else{
                evt.evt_type = BLE_BOILER_TEMP_CHAR_NOTIFICATION_DISABLED;
            }
            // Call the application event handler.
            p_cus->evt_handler(p_cus, &evt);
        }
    }
    // Writing to this Custom Value Characteristic: BLE_BOILER_CHAR_EVT_NEW_TEMPERATURE
    else if (p_evt_write->handle == p_cus->boilerTargetTemp_char_handles.value_handle)
    {
        evt.param_command.sBoilerTempTarget.ptr_data  = p_evt_write->data;
        evt.param_command.sBoilerTempTarget.length    = p_evt_write->len;
        evt.evt_type = BLE_BOILER_CHAR_EVT_NEW_TEMPERATURE;
        p_cus->evt_handler(p_cus, &evt);
    }

    // Writing to this Custom Value Characteristic: BLE_BREW_PRE_INFUSION_POWER_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->brewPreInfussionPower_char_handles.value_handle)
    {
        evt.param_command.sBrewPreInfussionPwr.ptr_data  = p_evt_write->data;
        evt.param_command.sBrewPreInfussionPwr.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_PRE_INFUSION_POWER_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_PRE_INFUSION_TIME__CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->brewPreInfussiontime__char_handles.value_handle)
    {
        evt.param_command.sBrewPreInfussionTmr.ptr_data  = p_evt_write->data;
        evt.param_command.sBrewPreInfussionTmr.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_PRE_INFUSION_TIME__CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_INFUSION_POWER_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->brewInfussionPower_char_handles.value_handle)
    {
        evt.param_command.sBrewInfussionPwr.ptr_data  = p_evt_write->data;
        evt.param_command.sBrewInfussionPwr.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_INFUSION_POWER_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_INFUSION_TIME__CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->brewInfussiontime__char_handles.value_handle)
    {
        evt.param_command.sBrewInfussionTmr.ptr_data  = p_evt_write->data;
        evt.param_command.sBrewInfussionTmr.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_INFUSION_TIME__CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_DECLINING_PR_POWER_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->brewDecliningPower_char_handles.value_handle)
    {
        evt.param_command.sBrewDecliningPwr.ptr_data  = p_evt_write->data;
        evt.param_command.sBrewDecliningPwr.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_DECLINING_PR_POWER_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_DECLINING_PR_TIME__CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->brewDecliningtime__char_handles.value_handle)
    {
        evt.param_command.sBrewDecliningTmr.ptr_data  = p_evt_write->data;
        evt.param_command.sBrewDecliningTmr.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_DECLINING_PR_TIME__CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Add PID parameters
    // Writing to this Custom Value Characteristic: PID_P_TERM_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_Pterm_char_handles.value_handle)
    {
        evt.param_command.sPid_P_term.ptr_data  = p_evt_write->data;
        evt.param_command.sPid_P_term.length    = p_evt_write->len;
        evt.evt_type = PID_P_TERM_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: PID_I_TERM_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_Iterm_char_handles.value_handle)
    {
        evt.param_command.sPid_I_term.ptr_data  = p_evt_write->data;
        evt.param_command.sPid_I_term.length    = p_evt_write->len;
        evt.evt_type = PID_I_TERM_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: PID_I_TERM_INT_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_ImaxTerm_char_handles.value_handle)
    {
        evt.param_command.sPid_Imax_term.ptr_data  = p_evt_write->data;
        evt.param_command.sPid_Imax_term.length    = p_evt_write->len;
        evt.evt_type = PID_I_TERM_INT_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: PID_I_TERM_WINDUP_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_Iwindup_char_handles.value_handle)
    {
        evt.param_command.sPid_Iwindup_term.ptr_data  = p_evt_write->data;
        evt.param_command.sPid_Iwindup_term.length    = p_evt_write->len;
        evt.evt_type = PID_I_TERM_WINDUP_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: PID_D_TERM_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_Dterm_char_handles.value_handle)
    {
        evt.param_command.sPid_D_term.ptr_data  = p_evt_write->data;
        evt.param_command.sPid_D_term.length    = p_evt_write->len;
        evt.evt_type = PID_D_TERM_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: PID_D_TERM_LPF_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_DlpfTerm_char_handles.value_handle)
    {
        evt.param_command.sPid_Dlpf_term.ptr_data  = p_evt_write->data;
        evt.param_command.sPid_Dlpf_term.length    = p_evt_write->len;
        evt.evt_type = PID_D_TERM_LPF_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: PID_GAIN___CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_GainTerm_char_handles.value_handle)
    {
        evt.param_command.sPid_Gain_term.ptr_data  = p_evt_write->data;
        evt.param_command.sPid_Gain_term.length    = p_evt_write->len;
        evt.evt_type = PID_GAIN___CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }else{}
}

/*****************************************************************************
 * Function: 	ble_cus_on_ble_evt
 * Description: Event that wil be forwarded from BLE stack too this service
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void ble_cus_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_cus_t * p_cus = (ble_cus_t *) p_context;
    
    NRF_LOG_INFO("BLE event received. Event type = %d\r\n", p_ble_evt->header.evt_id); 
    if (p_cus == NULL || p_ble_evt == NULL)
    {
        return;
    }
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_cus, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_cus, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_cus, p_ble_evt);
            break;
/* Handling this event is not necessary
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
            NRF_LOG_INFO("EXCHANGE_MTU_REQUEST event received.\r\n");
            break;
*/
        default:
            // No implementation needed.
            break;
    }
}


/*****************************************************************************
* Function: 	custom_value_char_add
* Description:  Function for adding the Custom Value characteristic.
* Caveats:      Youtube-TimeStamp: 48:50
* Parameters:	@param[in]   p_cus        Battery Service structure.
*               @param[in]   p_cus_init   Information needed to initialize the service.
* Return:       NRF_SUCCESS on success, otherwise an error code.
*****************************************************************************/
static uint32_t custom_value_char_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init, bleSpressoUserdata_struct * ptr_initVal)
{
    uint32_t              err_code;
    ble_add_char_params_t add_char_param;
    static uint8_t initValueChar[5] = {'0'}; 

    // Add BLE_CHAR_BLESPRESSO_STATUS characteristic
    uint8_t blespressoStatus[10] = {' '};
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BLESPRESSO_STATUS__UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;

    add_char_param.init_len         = 10;                      //init 5 bytes        
    add_char_param.max_len          = 10;
    add_char_param.p_init_value     = (uint8_t*)blespressoStatus;        //init value

    add_char_param.char_props.read  = 1;                      //Enable Read
    add_char_param.char_props.notify= 1;                      //Enable Notify

    add_char_param.read_access      = SEC_OPEN;
    add_char_param.cccd_write_access= SEC_OPEN;                //To be allow to enable or disable notification

    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->blespressoStatus_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }


    //uint8_t boilerWaterTemp[4] = {'0','0','0','0'};
    // Add Boilder Temperature Value characteristic
    fcn_FloatToChrArray(0.0f,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BOILER_WATER__TEMP_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;

    add_char_param.init_len         = 4;                      //init 5 bytes        
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;        //init value

    add_char_param.char_props.read  = 1;                      //Enable Read
    add_char_param.char_props.notify= 1;                      //Enable Notify

    add_char_param.read_access      = SEC_OPEN;
    add_char_param.cccd_write_access= SEC_OPEN;                //To be allow to enable or disable notification

    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->boilerTemp_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add Boilder Target Temperature characteristic
    //TimeStamp: 51:45
    //uint8_t boilerTargetTemp[4] = {'0','9','8','5'};
    fcn_FloatToChrArray(98.5f,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BOILER_TARGET_TEMP_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 4;                  //init 5 bytes        
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;

    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;

    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->boilerTargetTemp_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BREW_PRE_INFUSION_POWER characteristic
    //uint8_t brewPreInfusionPwr[3] = {'0','0','0'};
    fcn_FloatToChrArray(ptr_initVal->BrewPreInfussionPwr,(uint8_t*)&initValueChar[0],2,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BREW_PRE_INFUSION_POWER_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 3;                             
    add_char_param.max_len          = 3;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;
    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->brewPreInfussionPower_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BREW_PRE_INFUSION_TIME characteristic
    //uint8_t brewPreInfusiontmr[3] = {'0','0','0'};
    fcn_FloatToChrArray(ptr_initVal->BrewPreInfussionTmr,(uint8_t*)&initValueChar[0],2,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BREW_PRE_INFUSION_TIME__UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 3;                             
    add_char_param.max_len          = 3;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;
    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->brewPreInfussiontime__char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BREW_INFUSION_POWER characteristic
    //uint8_t brewInfusionPwr[4] = {'0','0','0', '0' };
    fcn_FloatToChrArray(ptr_initVal->BrewInfussionPwr,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BREW_INFUSION_POWER_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 4;                             
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;

    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->brewInfussionPower_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BREW_INFUSION_TIME characteristic
    //uint8_t brewInfusiontmr[3] = {'0','0','0'};
    fcn_FloatToChrArray(ptr_initVal->BrewInfussionTmr,(uint8_t*)&initValueChar[0],2,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BREW_INFUSION_TIME__UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 3;                             
    add_char_param.max_len          = 3;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;

    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->brewInfussiontime__char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BLE_CHAR_BREW_DECLINING_PR_POWER characteristic
    //uint8_t brewDecliningPressurePwr[4] = {'0','0','0', '0' };
    fcn_FloatToChrArray(ptr_initVal->BrewDecliningPwr,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BREW_DECLINING_PR_POWER_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 4;                             
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;

    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->brewDecliningPower_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BLE_CHAR_BREW_DECLINING_PR_TIME characteristic
    //uint8_t brewDecliningPressureTmr[3] = {'0','0','0'};
    fcn_FloatToChrArray(ptr_initVal->BrewDecliningTmr,(uint8_t*)&initValueChar[0],2,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BREW_DECLINING_PR_TIME__UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 3;                             
    add_char_param.max_len          = 3;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;

    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->brewDecliningtime__char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }
   
    return NRF_SUCCESS;
}

/*****************************************************************************
* Function: 	custom_value_PIDchar_add
* Description:  Function for adding the Custom Value characteristic.
* Caveats:      Youtube-TimeStamp: 48:50
* Parameters:	@param[in]   p_cus        Battery Service structure.
*               @param[in]   p_cus_init   Information needed to initialize the service.
* Return:       NRF_SUCCESS on success, otherwise an error code.
*****************************************************************************/
static uint32_t custom_value_PIDchar_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init, bleSpressoUserdata_struct *ptr_initVal)
{
    uint32_t              err_code;
    ble_add_char_params_t add_char_param;
    static uint8_t initValueChar[5] = {'0'}; 

    // Add PID_P_TERM characteristic
    //uint8_t pidPterm[4] = {'0' ,'0','0','0'};  //0.0
    fcn_FloatToChrArray(ptr_initVal->Pid_P_term,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_PID_P_TERM_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 4;                             
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;
    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->pid_Pterm_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add PID_I_TERM characteristic
    //uint8_t pidIterm[3] = {'0','0','0'};
    fcn_FloatToChrArray(ptr_initVal->Pid_I_term,(uint8_t*)&initValueChar[0],2,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_PID_I_TERM_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 3;                             
    add_char_param.max_len          = 3;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;
    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->pid_Iterm_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BLE_CHAR_PID_I_MAX characteristic
    //uint8_t pidImaxTerm[4] = {'0','0','0','0'};  //0.0
    fcn_FloatToChrArray(ptr_initVal->Pid_Imax_term,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_PID_I_MAX_TERM_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 4;                             
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;
    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->pid_ImaxTerm_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add PID_I_WINDUP_TERM characteristic   
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_PID_I_TERM_WINDUP_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 1;                             
    add_char_param.max_len          = 1;
    if(ptr_initVal->Pid_Iwindup_term == true)
    {
      initValueChar[0] = '1';
      add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    }else{
      initValueChar[0] = '0';
      add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    } 
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;
    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->pid_Iwindup_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BLE_CHAR_PID_D_TERM characteristic
    //uint8_t pidDterm[3] = {'0','0','0'};  //0.0  
    fcn_FloatToChrArray(ptr_initVal->Pid_D_term,(uint8_t*)&initValueChar[0],2,1);  
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_PID_D_TERM_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 3;                             
    add_char_param.max_len          = 3;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;
    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->pid_Dterm_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BLE_CHAR_PID_D_TERM_LPF characteristic
    //uint8_t pidDlpfTerm[4] = {'0','0','0','0'};  //0.0 
    fcn_FloatToChrArray(ptr_initVal->Pid_Dlpf_term,(uint8_t*)&initValueChar[0],3,1);   
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_PID_D_TERM_LPF_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 4;                             
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;
    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->pid_DlpfTerm_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BLE_CHAR_PID_D_TERM_LPF characteristic
    //uint8_t pidGainTerm[4] = {'0','0','0','0'};  //0.0  
    fcn_FloatToChrArray(ptr_initVal->Pid_Gain_term,(uint8_t*)&initValueChar[0],3,1);  
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_PID_GAIN___UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 4;                             
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;   //init value
    add_char_param.char_props.read  = 1;                  //Enable Read
    add_char_param.char_props.write = 1;                  //Enable write
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;
    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_param,
                                  &p_cus->pid_GainTerm_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }
   
    return NRF_SUCCESS;
}


/*****************************************************************************
* Function: 	ble_cus_init
* Description:  initialization of CUS BLE service
* Caveats:      Youtube-TimeStamp: 46:30
* Parameters:	
* Return:       NRF_SUCCESS on success otherwise an error code
*****************************************************************************/
uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
    uint32_t              err_code;
    ble_uuid_t            ble_uuid;

    // Initialize service structure
    p_cus->evt_handler        = p_cus_init->evt_handler;
    p_cus->conn_handle        = BLE_CONN_HANDLE_INVALID;

    // Add Custom Service UUID
    ble_uuid128_t base_uuid   = {CUSTOM_SERVICE_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
    //VERIFY_SUCCESS(err_code);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    // CREATE SERVICE FOR BREW CFG ///////////////////////////////////////////////////
    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = BLE_SERVICE_BLEESPRESSO_UUID;
    // Add the Custom Service to the database
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
    if (err_code != NRF_SUCCESS)
    {return err_code;}
    // Add Custom Value characteristic
    custom_value_char_add(p_cus, p_cus_init,(bleSpressoUserdata_struct *)&int_NvmData);

    // CREATE NEW SERVICE FOR PID ///////////////////////////////////////////////////
    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = BLE_SERVICE_PIDESPRESSO_UUID;
    // Add the Custom Service to the database
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
    if (err_code != NRF_SUCCESS)
    {return err_code;}

    // Add Custom Value characteristic
    return custom_value_PIDchar_add(p_cus, p_cus_init,(bleSpressoUserdata_struct *)&int_NvmData);

}

/****************************************************************************
* Function: 	ble_cus_custom_value_update
* Description:  This function update the temperature on the BLE: boilerTemp_char_handles Characteristic
* Caveats:      Youtube-TimeStamp: 53:00
* Parameters:	
* Return:       
*****************************************************************************/
uint32_t ble_cus_BoilerWaterTemperature_update(ble_cus_t * p_cus, uint8_t * ptr_waterTemp, uint16_t conn_handle)
{
    //NRF_LOG_INFO("BLE:  temp. update\r\n"); 
    if (p_cus == NULL)
    {
        return NRF_ERROR_NULL;
    }
    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_hvx_params_t params;
    uint16_t len = sizeof(ptr_waterTemp);

    // Initialize value struct.
    memset(&params, 0, sizeof(params));
    params.type   = BLE_GATT_HVX_NOTIFICATION;
    params.handle = p_cus->boilerTemp_char_handles.value_handle;
    params.p_data = ptr_waterTemp;
    params.p_len  = &len;

    // Update database.
    err_code = sd_ble_gatts_hvx(conn_handle, &params);
    if (err_code != NRF_SUCCESS)
    {  return err_code; }
    return err_code;
}
