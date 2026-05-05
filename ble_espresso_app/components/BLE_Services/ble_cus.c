#include "sdk_common.h"
#include "ble_cus.h"
#include <string.h>
#include "ble_srv_common.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"
#include "x04_Numbers.h"
#include "espressoMachineServices.h"

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
  
    // Writing to this Custom Value Characteristic: BLE_MACHINE_STATUS_CHAR_NOTIFY_
    if ( (p_evt_write->handle == p_cus->machine_status_char_handles.cccd_handle)  )
    {
        // CCCD written, call application event handler
        if (p_cus->evt_handler != NULL)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_MACHINE_STATUS_CHAR_NOTIFY_ENABLED;
            }else{
                evt.evt_type = BLE_MACHINE_STATUS_CHAR_NOTIFY_DISABLED;
            }
            // Call the application event handler.
            p_cus->evt_handler(p_cus, &evt);
        }
    }
    // Writing to this Custom Value Characteristic: BLE_MACHINE_BOILER_TEMP_CHAR_NOTIFY_
    else if ( (p_evt_write->handle == p_cus->boiler_water_temp_char_handles.cccd_handle)  )
    {
        // CCCD written, call application event handler
        if (p_cus->evt_handler != NULL)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_MACHINE_BOILER_TEMP_CHAR_NOTIFY_ENABLED;
            }else{
                evt.evt_type = BLE_MACHINE_BOILER_TEMP_CHAR_NOTIFY_DISABLED;
            }
            // Call the application event handler.
            p_cus->evt_handler(p_cus, &evt);
        }
    }
    // Writing to this Custom Value Characteristic: BLE_MACHINE_BOILER_SET_POINT_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->boiler_temp_set_point_char_handles.value_handle)
    {
        evt.param_command.Boiler_temp_set_point_s.ptr_data  = p_evt_write->data;
        evt.param_command.Boiler_temp_set_point_s.length    = p_evt_write->len;
        evt.evt_type = BLE_MACHINE_BOILER_SET_POINT_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_CHAR_BREW_TEMP_UUID
    else if (p_evt_write->handle == p_cus->brew_temp_char_handles.value_handle)
    {
        evt.param_command.Brew_temp_s.ptr_data  = p_evt_write->data;
        evt.param_command.Brew_temp_s.length    = p_evt_write->len;
        evt.evt_type = BLE_MACHINE_BREW_TEMP_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_CHAR_STEAM_TEMP_UUID
    else if (p_evt_write->handle == p_cus->brew_temp_char_handles.value_handle)
    {
        evt.param_command.Steam_temp_s.ptr_data  = p_evt_write->data;
        evt.param_command.Steam_temp_s.length    = p_evt_write->len;
        evt.evt_type = BLE_MACHINE_STEAM_TEMP_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }

    // Writing to this Custom Value Characteristic: BLE_BREW_PRE_INFUSION_POWER_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->preInfuse_power_char_handles.value_handle)
    {
        evt.param_command.PreInfusePwr_s.ptr_data  = p_evt_write->data;
        evt.param_command.PreInfusePwr_s.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_PRE_INFUSION_POWER_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_PRE_INFUSION_TIME__CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->preInfuse_time_char_handles.value_handle)
    {
        evt.param_command.PreInfuseTmr_s.ptr_data  = p_evt_write->data;
        evt.param_command.PreInfuseTmr_s.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_PRE_INFUSION_TIME__CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_INFUSION_POWER_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->infuse_power_char_handles.value_handle)
    {
        evt.param_command.InfusePwr_s.ptr_data  = p_evt_write->data;
        evt.param_command.InfusePwr_s.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_INFUSION_POWER_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_INFUSION_TIME__CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->infuse_time_char_handles.value_handle)
    {
        evt.param_command.InfuseTmr_s.ptr_data  = p_evt_write->data;
        evt.param_command.InfuseTmr_s.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_INFUSION_TIME__CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_DECLINING_PR_POWER_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->taper_power_char_handles.value_handle)
    {
        evt.param_command.TaperingPwr_s.ptr_data  = p_evt_write->data;
        evt.param_command.TaperingPwr_s.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_DECLINING_PR_POWER_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: BLE_BREW_DECLINING_PR_TIME__CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->taper_time_char_handles.value_handle)
    {
        evt.param_command.TaperingTmr_s.ptr_data  = p_evt_write->data;
        evt.param_command.TaperingTmr_s.length    = p_evt_write->len;
        evt.evt_type = BLE_BREW_DECLINING_PR_TIME__CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Add PID parameters
    // Writing to this Custom Value Characteristic: pidPTerm_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_p_term_char_handles.value_handle)
    {
        evt.param_command.PidPTerm_s.ptr_data  = p_evt_write->data;
        evt.param_command.PidPTerm_s.length    = p_evt_write->len;
        evt.evt_type = PID_P_TERM_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: pidITerm_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_i_term_char_handles.value_handle)
    {
        evt.param_command.PidITerm_s.ptr_data  = p_evt_write->data;
        evt.param_command.PidITerm_s.length    = p_evt_write->len;
        evt.evt_type = PID_I_TERM_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: pidITerm_INT_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_i_max_term_char_handles.value_handle)
    {
        evt.param_command.PidImaxTerm_s.ptr_data  = p_evt_write->data;
        evt.param_command.PidImaxTerm_s.length    = p_evt_write->len;
        evt.evt_type = PID_I_TERM_INT_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: pidITerm_WINDUP_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_Iwindup_char_handles.value_handle)
    {
        evt.param_command.PidIwindupTerm_s.ptr_data  = p_evt_write->data;
        evt.param_command.PidIwindupTerm_s.length    = p_evt_write->len;
        evt.evt_type = PID_I_TERM_WINDUP_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: pidDTerm_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_Dterm_char_handles.value_handle)
    {
        evt.param_command.PidDTerm_s.ptr_data  = p_evt_write->data;
        evt.param_command.PidDTerm_s.length    = p_evt_write->len;
        evt.evt_type = PID_D_TERM_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: pidDTerm_LPF_CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_DlpfTerm_char_handles.value_handle)
    {
        evt.param_command.PidDlpfTerm_s.ptr_data  = p_evt_write->data;
        evt.param_command.PidDlpfTerm_s.length    = p_evt_write->len;
        evt.evt_type = PID_D_TERM_LPF_CHAR_RX_EVT;
        p_cus->evt_handler(p_cus, &evt);
    }
    // Writing to this Custom Value Characteristic: PID_GAIN___CHAR_RX_EVT
    else if (p_evt_write->handle == p_cus->pid_GainTerm_char_handles.value_handle)
    {
        evt.param_command.PidGainTerm_s.ptr_data  = p_evt_write->data;
        evt.param_command.PidGainTerm_s.length    = p_evt_write->len;
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
    #if NRF_LOG_CUS_EVT_ENABLED == 1
      NRF_LOG_INFO("BLE event received. Event type = %d\r\n", p_ble_evt->header.evt_id); 
    #endif
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
/* Handling this event is not necessary*/
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
            NRF_LOG_INFO("EXCHANGE_MTU_REQUEST event received.\r\n");
            break;

        default:
            // No implementation needed.
            break;
    }
}


/*****************************************************************************
* Function: 	ble_cus_espresso_char_add
* Description:  Function for adding the Custom Value characteristic.
* Caveats:      Youtube-TimeStamp: 48:50
* Parameters:	@param[in]   p_cus        Battery Service structure.
*               @param[in]   p_cus_init   Information needed to initialize the service.
* Return:       NRF_SUCCESS on success, otherwise an error code.
*****************************************************************************/
static uint32_t ble_cus_espresso_char_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init, espresso_user_config_t * ptr_initVal)
{
    uint32_t              err_code;
    ble_add_char_params_t add_char_param;
    static uint8_t initValueChar[5] = {'0'}; 
    
    /*<NOTIFICATION + READ> 
    Add BLE_CHAR_BLESPRESSO_STATUS characteristic
    TODO
    */
    uint8_t blespressoStatus[10] = {' '};
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_MACHINE_STATUS__UUID;   //set UUID
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
                                  &p_cus->machine_status_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }


    
    /*<NOTIFICATION + READ> 
    Add Boilder WATER Temperature Value characteristic
    uint8_t boilerWaterTemp[4] = {'0','0','0','0'}; */
    fcn_FloatToChrArray(0.0f,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BOILER_WATER_TEMP_UUID;   //set UUID
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
                                  &p_cus->boiler_water_temp_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    /*<READ + WRITE>
    Add Boilder SET POINT Temperature characteristic
    TimeStamp: 51:45
    uint8_t boilerTargetTemp[4] = {'0','9','8','5'}; */
    fcn_FloatToChrArray(ptr_initVal->boilerTempSetpointDegC,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BOILER_SET_POINT_TEMP_UUID;   //set UUID
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
                                  &p_cus->boiler_temp_set_point_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    /*<READ + WRITE> 
    BLE_CHAR_BREW_TEMP_UUID
    Add BREW Temperature PRESET characteristic
    This separate char allows the app to configure BREW setpoint independently */
    fcn_FloatToChrArray(ptr_initVal->brewTempDegC,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_BREW_TEMP_UUID;
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 4;
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;
    add_char_param.char_props.read  = 1;
    add_char_param.char_props.write = 1;
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;

    err_code = characteristic_add(p_cus->service_handle,
                                  &add_char_param,
                                  &p_cus->brew_temp_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    /*<READ + WRITE> 
    BLE_CHAR_STEAM_TEMP_UUID
    Add STEAM Temperature PRESET characteristic
    This separate char allows the app to configure STEAM setpoint independently */
    fcn_FloatToChrArray(ptr_initVal->steamTempDegC,(uint8_t*)&initValueChar[0],3,1);
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_STEAM_TEMP_UUID;
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 4;
    add_char_param.max_len          = 4;
    add_char_param.p_init_value     = (uint8_t*)initValueChar;
    add_char_param.char_props.read  = 1;
    add_char_param.char_props.write = 1;
    add_char_param.char_props.notify= 0;
    add_char_param.read_access      = SEC_OPEN;
    add_char_param.write_access     = SEC_OPEN;

    err_code = characteristic_add(p_cus->service_handle,
                                  &add_char_param,
                                  &p_cus->steam_temp_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    /*<READ + WRITE>
    Add BREW_PRE_INFUSION_POWER characteristic
    uint8_t brewPreInfusionPwr[3] = {'0','0','0'};*/
    fcn_FloatToChrArray(ptr_initVal->profPreInfusePwr,(uint8_t*)&initValueChar[0],2,1);
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
                                  &p_cus->preInfuse_power_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    /*<READ + WRITE>
    Add BREW_PRE_INFUSION_TIME characteristic
    uint8_t brewPreInfusiontmr[3] = {'0','0','0'};*/
    fcn_FloatToChrArray(ptr_initVal->profPreInfuseTmr,(uint8_t*)&initValueChar[0],2,1);
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
                                  &p_cus->preInfuse_time_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    /*<READ + WRITE>
    Add BREW_INFUSION_POWER characteristic
    uint8_t brewInfusionPwr[4] = {'0','0','0', '0' };*/
    fcn_FloatToChrArray(ptr_initVal->profInfusePwr,(uint8_t*)&initValueChar[0],3,1);
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
                                  &p_cus->infuse_power_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }
  
    /*<READ + WRITE>
    Add BREW_INFUSION_TIME characteristic
    uint8_t brewInfusiontmr[3] = {'0','0','0'};*/
    fcn_FloatToChrArray(ptr_initVal->profInfuseTmr,(uint8_t*)&initValueChar[0],2,1);
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
                                  &p_cus->infuse_time_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    /*<READ + WRITE>
    Add BLE_CHAR_BREW_DECLINING_PR_POWER characteristic
    uint8_t brewDecliningPressurePwr[4] = {'0','0','0', '0' };*/
    fcn_FloatToChrArray(ptr_initVal->profTaperingPwr,(uint8_t*)&initValueChar[0],3,1);
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
                                  &p_cus->taper_power_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    /*<READ + WRITE>
    Add BLE_CHAR_BREW_DECLINING_PR_TIME characteristic
    uint8_t brewDecliningPressureTmr[3] = {'0','0','0'};*/
    fcn_FloatToChrArray(ptr_initVal->profTaperingTmr,(uint8_t*)&initValueChar[0],2,1);
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
                                  &p_cus->taper_time_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }
   
    return NRF_SUCCESS;
}

/*****************************************************************************
* Function: 	ble_cus_controller_char_add
* Description:  Function for adding the Custom Value characteristic.
* Caveats:      Youtube-TimeStamp: 48:50
* Parameters:	@param[in]   p_cus        Battery Service structure.
*               @param[in]   p_cus_init   Information needed to initialize the service.
* Return:       NRF_SUCCESS on success, otherwise an error code.
*****************************************************************************/
static uint32_t ble_cus_controller_char_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init, espresso_user_config_t *ptr_initVal)
{
    uint32_t              err_code;
    ble_add_char_params_t add_char_param;
    static uint8_t initValueChar[5] = {'0'}; 

    // Add pidPTerm characteristic
    //uint8_t pidPterm[4] = {'0' ,'0','0','0'};  //0.0
    fcn_FloatToChrArray(ptr_initVal->pidPTerm,(uint8_t*)&initValueChar[0],3,1);
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
                                  &p_cus->pid_p_term_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add pidITerm characteristic
    //uint8_t pidIterm[3] = {'0','0','0'};
    fcn_FloatToChrArray(ptr_initVal->pidITerm,(uint8_t*)&initValueChar[0],2,1);
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
                                  &p_cus->pid_i_term_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add BLE_CHAR_PID_I_MAX characteristic
    //uint8_t pidImaxTerm[4] = {'0','0','0','0'};  //0.0
    fcn_FloatToChrArray(ptr_initVal->pidImaxTerm,(uint8_t*)&initValueChar[0],3,1);
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
                                  &p_cus->pid_i_max_term_char_handles);
    if (err_code != NRF_SUCCESS)
    { return err_code;  }

    // Add PID_I_WINDUP_TERM characteristic   
    memset(&add_char_param, 0, sizeof(add_char_param));
    add_char_param.uuid             = BLE_CHAR_PID_I_TERM_WINDUP_UUID;   //set UUID
    add_char_param.uuid_type        = p_cus->uuid_type;
    add_char_param.init_len         = 1;                             
    add_char_param.max_len          = 1;
    if(ptr_initVal->pidIwindupTerm == true)
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

    // Add BLE_CHAR_pidDTerm characteristic
    //uint8_t pidDterm[3] = {'0','0','0'};  //0.0  
    fcn_FloatToChrArray(ptr_initVal->pidDTerm,(uint8_t*)&initValueChar[0],2,1);  
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

    // Add BLE_CHAR_pidDTerm_LPF characteristic
    //uint8_t pidDlpfTerm[4] = {'0','0','0','0'};  //0.0 
    fcn_FloatToChrArray(ptr_initVal->pidDlpfTerm,(uint8_t*)&initValueChar[0],3,1);   
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

    // Add BLE_CHAR_pidDTerm_LPF characteristic
    //uint8_t pidGainTerm[4] = {'0','0','0','0'};  //0.0  
    fcn_FloatToChrArray(ptr_initVal->pidGainTerm,(uint8_t*)&initValueChar[0],3,1);  
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
uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init, espresso_user_config_t* ptr_init_data)
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
    /*  CREATE BLE CUSTOM SERVICE: ESPRESSO MACHINE AND PROFILE */
    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = BLE_SERVICE_ESPRESSO_MACHINE_UUID;
    // Add the Custom Service to the database
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
    if (err_code != NRF_SUCCESS)
    {return err_code;}
    // Add Custom Value characteristic
    //ble_cus_espresso_char_add(p_cus, p_cus_init,(espresso_user_config_t *)&g_Espresso_user_config_s);
    ble_cus_espresso_char_add(p_cus, p_cus_init,ptr_init_data);

    /*  CREATE BLE CUSTOM SERVICE: CONTROLLER */
    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = BLE_SERVICE_CONTROLLER_UUID;
    // Add the Custom Service to the database
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
    if (err_code != NRF_SUCCESS)
    {return err_code;}

    // Add Custom Value characteristic
    //return ble_cus_controller_char_add(p_cus, p_cus_init,(espresso_user_config_t *)&g_Espresso_user_config_s);
    return ble_cus_controller_char_add(p_cus, p_cus_init,ptr_init_data);
}

/****************************************************************************
* Function: 	ble_cus_notify_boiler_water_temp
* Description:  This function update the temperature on the BLE: boiler_water_temp_char_handles Characteristic
* Caveats:      Youtube-TimeStamp: 53:00
* Parameters:	
* Return:       
*****************************************************************************/
uint32_t ble_cus_notify_boiler_water_temp(ble_cus_t * p_cus, uint8_t * ptr_waterTemp, uint16_t conn_handle)
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
    params.handle = p_cus->boiler_water_temp_char_handles.value_handle;
    params.p_data = ptr_waterTemp;
    params.p_len  = &len;

    // Update database.
    err_code = sd_ble_gatts_hvx(conn_handle, &params);
    if (err_code != NRF_SUCCESS)
    {  return err_code; }
    return err_code;
}
