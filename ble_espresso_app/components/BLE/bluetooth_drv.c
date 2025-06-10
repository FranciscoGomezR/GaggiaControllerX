
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "bluetooth_drv.h"
#include "BLEspressoServices.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "x04_Numbers.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************
BLE_CUS_DEF(m_cus);
//BLE_CUS_DEF(m_PIDcus);

//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
volatile uint8_t DataReceived[4];
volatile uint32_t iTagertTemp;
volatile float iTagertTemp2;
volatile uint8_t dataLen;

volatile uint8_t flg_BrewCfg,flg_PidCfg,flg_ReadCfg;

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
volatile bleSpressoUserdata_struct read_NvmData;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */
/* YOUR_JOB: Declare all services structure your application is using
 *  BLE_XYZ_DEF(m_xyz);
 */

 // YOUR_JOB: Use UUIDs for service(s) used in your application.
static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
};

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name);
static void pm_evt_handler(pm_evt_t const * p_evt);
static void gap_params_init(void);
static void gatt_init(void);
static void nrf_qwr_error_handler(uint32_t nrf_error);
static void services_init(void);
static void db_discovery_init(void);
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt);
static void conn_params_error_handler(uint32_t nrf_error);
static void conn_params_init(void);
static void on_adv_evt(ble_adv_evt_t ble_adv_evt);
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void ble_stack_init(void);
static void peer_manager_init(void);
static void delete_bonds(void);
static void advertising_init(void);
static void power_management_init(void);

//CUS service
static void cus_evt_handler(ble_cus_t * p_cus, ble_cus_evt_t * p_evt);

//***********************************************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//***********************************************************************************************************
/*****************************************************************************
 * Function: 	BLE_bluetooth_init
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void BLE_bluetooth_init(void)
{
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();
    peer_manager_init();

    flg_BrewCfg=0;
    flg_PidCfg=0;
    flg_ReadCfg=0;
}


/*****************************************************************************
 * Function: 	advertising_start
 * Description: Function for starting advertising.
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETED_SUCEEDED event
    }
    else
    {
        ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);

        APP_ERROR_CHECK(err_code);
    }
}

/*****************************************************************************
 * Function: 	ble_disconnect
 * Description: Function for disconnect link, only when it's connected.
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void ble_disconnect(void)
{
  ret_code_t err_code;

  if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
  {
    err_code = sd_ble_gap_disconnect(m_conn_handle,BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_ERROR_INVALID_STATE)
    {
        APP_ERROR_CHECK(err_code);
    }
  }

}

/*****************************************************************************
 * Function: 	ble_restart_without_whitelist
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void ble_restart_without_whitelist(void)
{
    ret_code_t err_code;
    if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        err_code = ble_advertising_restart_without_whitelist(&m_advertising);
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
    }
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
void sleep_mode_enter(void)
{
    ret_code_t err_code;

    //err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    //APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

//***********************************************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//***********************************************************************************************************

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_disconnect_on_sec_failure(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            advertising_start(false);
            break;

        default:
            break;
    }
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    /* YOUR_JOB: Use an appearance value matching the application's use case.
       err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_);
       APP_ERROR_CHECK(err_code); */

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}



/**@brief Function for handling the YYY Service events.
 * YOUR_JOB implement a service handler function depending on the event the service you are using can generate
 *
 * @details This function will be called for all YY Service events which are passed to
 *          the application.
 *
 * @param[in]   p_yy_service   YY Service structure.
 * @param[in]   p_evt          Event received from the YY Service.
 *
 *
static void on_yys_evt(ble_yy_service_t     * p_yy_service,
                       ble_yy_service_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_YY_NAME_EVT_WRITE:
            APPL_LOG("[APPL]: charact written with value %s. ", p_evt->params.char_xx.value.p_str);
            break;

        default:
            // No implementation needed.
            break;
    }
}
*/


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize CUS.
    ble_cus_init_t    cus_init = {0};
    cus_init.evt_handler  = cus_evt_handler;

    err_code = ble_cus_init(&m_cus, &cus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
            //err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            //APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;

        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("Disconnected.");
            #endif
            // LED indication will be changed when advertising starts.
            break;

        case BLE_GAP_EVT_CONNECTED:
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("Connected.");
            #endif
            //err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            //APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_DEBUG("PHY update request.");
            #endif
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_DEBUG("GATT Client Timeout.");
            #endif
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_DEBUG("GATT Server Timeout.");
            #endif
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;
    #if NRF_LOG_ENABLED == 1
    NRF_LOG_INFO("Erase bonds!");
    #endif
    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    #ifdef APP_ADV_DURATION
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    #endif
    init.evt_handler = on_adv_evt;
    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);
    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/*****************************************************************************
 * Function: 	ble_cuse_evt
 * Description: Event that wil be forwarded from BLE stack too this service
 * Caveats:     timeStamp < 1:30
 * Parameters:	
 * Return:
 *****************************************************************************/
static void cus_evt_handler(ble_cus_t * p_cus, ble_cus_evt_t * p_evt)
{
    ret_code_t  err_code;
    #if NRF_LOG_ENABLED == 1
    NRF_LOG_DEBUG("Event %d",p_evt->evt_type);
    #endif
    switch(p_evt->evt_type)
    {
    //Event -> New target temperature for the boiler
        case BLE_BOILER_CHAR_EVT_NEW_TEMPERATURE:
            //Youtube-TimeStamp: 2:33:00
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_DEBUG("BLE -> New target Temperature");
            #endif
            BLEspressoVar.TargetBoilerTemp = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sBoilerTempTarget.ptr_data,3,1);              
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m TARGET Temp: %d . %d \r\n \033[0;40m", (int)BLEspressoVar.TargetBoilerTemp);
            #endif
        break;
     //Event -> Enable notification to get water temperature
        case BLE_BOILER_TEMP_CHAR_NOTIFICATION_ENABLED:
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_DEBUG("BLE -> Boiler Water Temp Notifications ENABLE");
            #endif
        break;
    //Event -> Disable notification to get water temperature
        case BLE_BOILER_TEMP_CHAR_NOTIFICATION_DISABLED:
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_DEBUG("BLE -> Boiler Water Temp Notifications DISABLE");
            #endif
        break;
    //Event -> Enable notification to get machine status
        case BLESPRESSO_STATUS_CHAR_NOTIFICATION_ENABLED:
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_DEBUG("BLE -> Blespresso Status Notifications ENABLE");
            #endif
        break;
    //Event -> Disbale notification to get machine status
        case BLESPRESSO_STATUS_CHAR_NOTIFICATION_DISABLED:
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_DEBUG("BLE -> Blespresso Status Notifications DISABLE");
            #endif
        break;
    //Event -> Get new value from mobile for char:  BREW_PRE_INFUSION_POWER
        case BLE_BREW_PRE_INFUSION_POWER_CHAR_RX_EVT:
            BLEspressoVar.BrewPreInfussionPwr = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sBrewPreInfussionPwr.ptr_data,2,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m PreInfusion POWER: %d \r\n \033[0;40m", (int)BLEspressoVar.BrewPreInfussionPwr);
            #endif
        break;
    //Event -> Get new value from mobile for char:  BREW_PRE_INFUSION_TIME
        case BLE_BREW_PRE_INFUSION_TIME__CHAR_RX_EVT:
            BLEspressoVar.BrewPreInfussionTmr = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sBrewPreInfussionTmr.ptr_data,2,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m PreInfusion TIME: %d \r\n \033[0;40m", (int)BLEspressoVar.BrewPreInfussionTmr);
            #endif
        break;
    //Event -> Get new value from mobile for char:  BREW_INFUSION_POWER
        case BLE_BREW_INFUSION_POWER_CHAR_RX_EVT:   
            BLEspressoVar.BrewInfussionPwr = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sBrewInfussionPwr.ptr_data,3,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m Infusion POWER: %d \r\n \033[0;40m", (int)BLEspressoVar.BrewInfussionPwr);
            #endif
        break;
     //Event -> Get new value from mobile for char:  BREW_INFUSION_TIME
        case BLE_BREW_INFUSION_TIME__CHAR_RX_EVT:
            BLEspressoVar.BrewInfussionTmr = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sBrewInfussionTmr.ptr_data,2,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m Infusion TIME: %d \r\n \033[0;40m", (int)BLEspressoVar.BrewInfussionTmr);
            #endif
        break;
    //Event -> Get new value from mobile for char:  BREW_DECLINING_PR_POWER
        case BLE_BREW_DECLINING_PR_POWER_CHAR_RX_EVT:
            BLEspressoVar.BrewDecliningPwr = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sBrewDecliningPwr.ptr_data,3,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m Declining Pressure POWER: %d \r\n \033[0;40m", (int)BLEspressoVar.BrewDecliningPwr);
            #endif
        break;
    //Event -> Get new value from mobile for char:  BREW_DECLINING_PR_TIME
        case BLE_BREW_DECLINING_PR_TIME__CHAR_RX_EVT:
            BLEspressoVar.BrewDecliningTmr = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sBrewDecliningTmr.ptr_data,2,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m Declining Pressure TIME: %d \r\n \033[0;40m", (int)BLEspressoVar.BrewDecliningTmr);
            #endif
            flg_BrewCfg =1;
        break;
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //Event -> Get new value from mobile for char:  PID_P_TERM
        case PID_P_TERM_CHAR_RX_EVT:
            BLEspressoVar.Pid_P_term = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sPid_P_term.ptr_data,3,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m P Term: %d \r\n \033[0;40m", (int)BLEspressoVar.Pid_P_term);
            #endif
        break;
    //Event -> Get new value from mobile for char:  PID_I_TERM
        case PID_I_TERM_CHAR_RX_EVT:
            BLEspressoVar.Pid_I_term = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sPid_I_term.ptr_data,2,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m I term: %d \r\n \033[0;40m", (int)BLEspressoVar.Pid_I_term);
            #endif
        break;
    //Event -> Get new value from mobile for char:  PID_I_TERM
        case PID_I_TERM_INT_CHAR_RX_EVT:
            BLEspressoVar.Pid_Imax_term = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sPid_Imax_term.ptr_data,3,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m Imax Term: %d \r\n \033[0;40m", (int)BLEspressoVar.Pid_Imax_term);
            #endif
        break;
     //Event -> Get new value from mobile for char:  PID_I_TERM_WINDUP
        case PID_I_TERM_WINDUP_CHAR_RX_EVT:
            if(iTagertTemp == 0)
            {
              BLEspressoVar.Pid_Iwindup_term = false;
              #if NRF_LOG_ENABLED == 1
              NRF_LOG_INFO("\033[0;36m I windup Term: FALSE \r\n \033[0;40m");
              #endif
            }else if (iTagertTemp == 1)
            {
              BLEspressoVar.Pid_Iwindup_term = true;
              #if NRF_LOG_ENABLED == 1
              NRF_LOG_INFO("\033[0;36m I windup Term: TRUE \r\n \033[0;40m");
              #endif
            }else{
              BLEspressoVar.Pid_Iwindup_term = true;
              #if NRF_LOG_ENABLED == 1
              NRF_LOG_INFO("\033[0;36m I windup Term: TRUE \r\n \033[0;40m");
              #endif
            } 
            
        break;
     //Event -> Get new value from mobile for char:  PID_D_TERM
        case PID_D_TERM_CHAR_RX_EVT:
            BLEspressoVar.Pid_D_term = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sPid_D_term.ptr_data,2,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m D term: %d \r\n \033[0;40m", (int)BLEspressoVar.Pid_D_term);
            #endif
        break;
     //Event -> Get new value from mobile for char:  PID_D_TERM_LPF
        case PID_D_TERM_LPF_CHAR_RX_EVT:
            BLEspressoVar.Pid_Dlpf_term = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sPid_Dlpf_term.ptr_data,3,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m D Low-Pass Filter: %d \r\n \033[0;40m", (int)BLEspressoVar.Pid_Dlpf_term);
            #endif
        break;
     //Event -> Get new value from mobile for char:  PID_GAIN
        case PID_GAIN___CHAR_RX_EVT:
            BLEspressoVar.Pid_Gain_term = (float) fcn_ChrArrayToFloat((char *)p_evt->param_command.sPid_Gain_term.ptr_data,3,1);
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m PID Gain: %d \r\n \033[0;40m", (int)BLEspressoVar.Pid_Gain_term);
            #endif
            flg_PidCfg = 1;
        break;

        default:
            // No implementation needed.
            #if NRF_LOG_ENABLED == 1
            NRF_LOG_INFO("\033[0;36m ble_cuse_evt  un-recognize cus service \r\n \033[0;40m");
            #endif
            break;
    }

}

/*****************************************************************************
* Function: 	ble_update_boilerWaterTemp
* Description:  Sen data: boilerWaterTemp to Mobile
* Caveats:      Youtube-TimeStamp: 2:10:00  - 2:26:00
* Parameters:	
* Return:       
*****************************************************************************/
void ble_update_boilerWaterTemp(float waterTemp)
{
      ret_code_t err_code;
      uint32_t intTemperature = (uint32_t)(waterTemp * 10.0f); 
      uint8_t sTemp[4] = {0};
      uint8_t sbleTemp[4] = {'0','0','0','0'};
      sprintf(sTemp, "%d", intTemperature);
      uint8_t len;
      len = strlen(sTemp);
      switch(len)
      {
          case 1:
            sbleTemp[3] = sTemp[0];
          break;

          case 2:
            sbleTemp[2] = sTemp[0];
            sbleTemp[3] = sTemp[1];
          break;

          case 3:
            sbleTemp[1] = sTemp[0];
            sbleTemp[2] = sTemp[1];
            sbleTemp[3] = sTemp[2];
          break;

          case 4:
            sbleTemp[0] = sTemp[0];
            sbleTemp[1] = sTemp[1];
            sbleTemp[2] = sTemp[2];
            sbleTemp[3] = sTemp[3];
          break;
      }
      err_code = ble_cus_BoilerWaterTemperature_update(&m_cus, sbleTemp, m_conn_handle);

}


/*
iTagertTemp = 0;
iTagertTemp2 = fcn_ChrArrayToFloat((char *)p_evt->param_command.sBoilerTempTarget.ptr_data,3,1);
iTagertTemp += (uint32_t)((*p_evt->param_command.sBoilerTempTarget.ptr_data -48) * 1000);
p_evt->param_command.sBoilerTempTarget.ptr_data++;
iTagertTemp += (uint32_t)((*p_evt->param_command.sBoilerTempTarget.ptr_data -48) * 100);
p_evt->param_command.sBoilerTempTarget.ptr_data++;
iTagertTemp += (uint32_t)((*p_evt->param_command.sBoilerTempTarget.ptr_data -48) * 10);
p_evt->param_command.sBoilerTempTarget.ptr_data++;
iTagertTemp += (uint32_t)((*p_evt->param_command.sBoilerTempTarget.ptr_data -48));
BLEspressoVar.TargetBoilerTemp = (float) iTagertTemp/10.0f;
*/







