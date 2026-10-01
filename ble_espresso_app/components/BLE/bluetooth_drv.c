
/*******************************************************************************
 *
 *		INCLUDE FILE SECTION FOR THIS MODULE
 *
 ******************************************************************************/
#include "bluetooth_drv.h"
#include "espressoMachineServices.h"
#include "nrf_log.h"
#include "app_util.h"
#include "x04_Numbers.h"

/*******************************************************************************
 *
 *		PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
 *
 ******************************************************************************/
BLE_CUS_DEF(m_cus);
/*BLE_CUS_DEF(m_PIDcus);*/

/* First/last config-write event; rows of BLE_CFG_CHAR_arr map 1:1 to this range */
#define BLE_CFG_EVT_FIRST       BLE_MACHINE_BOILER_SET_POINT_CHAR_RX_EVT
#define BLE_CFG_EVT_LAST        PID_I_BOOST_CHAR_RX_EVT
#define BLE_CFG_CHAR_CNT        ((uint32_t)BLE_CFG_EVT_LAST - (uint32_t)BLE_CFG_EVT_FIRST + 1U)
/* Longest config payload: 3 integer digits + 1 decimal digit */
#define BLE_CFG_PAYLOAD_MAX     4U

/*******************************************************************************
 *
 *		PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
 *
 ******************************************************************************/
/* One writable config characteristic: target field, valid range, ASCII width
 * and the NVM region that must be saved when the field changes. */
typedef struct
{
  volatile float *ptrValue;     /* field in g_Espresso_user_config_s           */
  float           minVal;       /* lowest accepted value                       */
  float           maxVal;       /* highest accepted value                      */
  uint8_t         intDigits;    /* ASCII integer digits; payload adds 1 decimal */
  uint8_t         pendingBit;   /* BLE_CFG_PENDING_SHOT / _CTRL / _NONE        */
} ble_cfg_char_t;

/*******************************************************************************
 *
 *		PUBLIC VARIABLES
 *
 ******************************************************************************/
/* BLE config "pending save" state.
 * g_ble_cfg_pending_mask: one bit per NVM region changed over BLE and not yet
 *   saved. Set in BLE IRQ, cleared by main loop (critical region) after a
 *   verified NVM write. Also picks which controller is reloaded live.
 * g_ble_cfg_quiet_secs: seconds since the last accepted BLE config write.
 *   Reset in BLE IRQ, incremented (saturating) by the 1 s main-loop task.
 *   Save runs once it reaches BLE_CFG_SAVE_DEBOUNCE_SECS, so a burst of char
 *   writes from the app costs one sector erase. */
volatile uint8_t g_ble_cfg_pending_mask = BLE_CFG_PENDING_NONE;
volatile uint8_t g_ble_cfg_quiet_secs   = 0U;

/*******************************************************************************
 *
 *		PRIVATE VARIABLES
 *
 ******************************************************************************/
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */
/* YOUR_JOB: Declare all services structure your application is using
 *  BLE_XYZ_DEF(m_xyz);
 */

 /* YOUR_JOB: Use UUIDs for service(s) used in your application. */
static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
};

/** Lookup table for every writable config characteristic.
 *  Row index = (RX event type - BLE_MACHINE_BOILER_SET_POINT_CHAR_RX_EVT).
 *  Each row gives the target field, allowed range, ASCII width and which NVM
 *  region (SHOT / CTRL / NONE) must be saved when the field changes.
 *  Row order MUST match ble_cus_evt_type_t (checked by STATIC_ASSERT). */
static const ble_cfg_char_t BLE_CFG_CHAR_arr[] =
{
  /* 0x1403 active boiler setpoint: RAM only, not stored in NVM */
  { &g_Espresso_user_config_s.boilerTempSetpointDegC,
    BOILER_SETPOINT_TEMP_MIN_DEGC, BOILER_SETPOINT_TEMP_MAX_DEGC, 3U, BLE_CFG_PENDING_NONE },
  /* 0x1404 brew temperature preset */
  { &g_Espresso_user_config_s.brewTempDegC,
    BREW_TEMP_MIN_DEGC, BREW_TEMP_MAX_DEGC, 3U, BLE_CFG_PENDING_SHOT },
  /* 0x1405 steam temperature preset */
  { &g_Espresso_user_config_s.steamTempDegC,
    STEAM_TEMP_MIN_DEGC, STEAM_TEMP_MAX_DEGC, 3U, BLE_CFG_PENDING_SHOT },
  /* 0x1406 pre-infusion pump power */
  { &g_Espresso_user_config_s.profPreInfusePwr,
    PROF_PREINFUSE_PWR_MIN_PWR, PROF_PREINFUSE_PWR_MAX_PWR, 2U, BLE_CFG_PENDING_SHOT },
  /* 0x1407 pre-infusion time */
  { &g_Espresso_user_config_s.profPreInfuseTmr,
    PROF_PREINFUSE_TMR_MIN_SECS, PROF_PREINFUSE_TMR_MAX_SECS, 2U, BLE_CFG_PENDING_SHOT },
  /* 0x1408 infusion pump power */
  { &g_Espresso_user_config_s.profInfusePwr,
    PROF_INFUSE_PWR_MIN_PWR, PROF_INFUSE_PWR_MAX_PWR, 3U, BLE_CFG_PENDING_SHOT },
  /* 0x1409 infusion time */
  { &g_Espresso_user_config_s.profInfuseTmr,
    PROF_INFUSE_TMR_MIN_SECS, PROF_INFUSE_TMR_MAX_SECS, 2U, BLE_CFG_PENDING_SHOT },
  /* 0x140A declining-pressure pump power */
  { &g_Espresso_user_config_s.profTaperingPwr,
    PROF_TAPERING_PWR_MIN_PWR, PROF_TAPERING_PWR_MAX_PWR, 3U, BLE_CFG_PENDING_SHOT },
  /* 0x140B declining-pressure time */
  { &g_Espresso_user_config_s.profTaperingTmr,
    PROF_TAPERING_TMR_MIN_SECS, PROF_TAPERING_TMR_MAX_SECS, 2U, BLE_CFG_PENDING_SHOT },
  /* 0x1501 PID P term */
  { &g_Espresso_user_config_s.pidPTerm,
    PID_P_TERM_MIN, PID_P_TERM_MAX, 3U, BLE_CFG_PENDING_CTRL },
  /* 0x1502 PID I term */
  { &g_Espresso_user_config_s.pidITerm,
    PID_I_TERM_MIN, PID_I_TERM_MAX, 2U, BLE_CFG_PENDING_CTRL },
  /* 0x1503 PID I max */
  { &g_Espresso_user_config_s.pidImaxTerm,
    PID_I_MAX_TERM_MIN, PID_I_MAX_TERM_MAX, 3U, BLE_CFG_PENDING_CTRL },
  /* 0x1504 PID D term */
  { &g_Espresso_user_config_s.pidDTerm,
    PID_D_TERM_MIN, PID_D_TERM_MAX, 2U, BLE_CFG_PENDING_CTRL },
  /* 0x1505 PID P boost */
  { &g_Espresso_user_config_s.pidPboostTerm,
    PID_P_BOOST_TERM_MIN, PID_P_BOOST_TERM_MAX, 2U, BLE_CFG_PENDING_CTRL },
  /* 0x1506 PID I boost */
  { &g_Espresso_user_config_s.pidIboostTerm,
    PID_I_BOOST_TERM_MIN, PID_I_BOOST_TERM_MAX, 2U, BLE_CFG_PENDING_CTRL }
};
STATIC_ASSERT(ARRAY_SIZE(BLE_CFG_CHAR_arr) == BLE_CFG_CHAR_CNT);

/*******************************************************************************
 *
 *		PRIVATE FUNCTIONS PROTOYPES
 *
 ******************************************************************************/
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name);
static void pm_evt_handler(pm_evt_t const * p_evt);
static void gap_params_init(void);
static void gatt_init(void);
static void nrf_qwr_error_handler(uint32_t nrf_error);

static void services_init(espresso_user_config_t* ptr_init_data);

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

/* CUS service */
static void cus_evt_handler(ble_cus_t * p_cus, ble_cus_evt_t * p_evt);
static bool parse_ble_ascii_float(const struct_CharData *ptr_char, uint8_t int_digits,
                                  float min_val, float max_val, float *ptr_out);
static void restore_gatt_value(const struct_CharData *ptr_char, const ble_cfg_char_t *ptr_row);

/*******************************************************************************
 *
 *		PUBLIC FUNCTIONS SECTION
 *
 ******************************************************************************/
/*****************************************************************************
 * Function: 	BLE_bluetooth_init
 * Description:
 * Caveats:
 * Parameters:
 * Return:
 *****************************************************************************/
void bluetooth_low_energy_init(espresso_user_config_t* ptr_init_data)
{
  power_management_init();
  ble_stack_init();
  gap_params_init();
  gatt_init();
  /* FCN: services_init create the Custom service for: BLE espresso app
  by calling FCN: ble_cus_init(&m_cus, &cus_init);
  The services created are:
  - SERVICE FOR BREW
  - SERVICE FOR PID
  Each service for each window in the phone app */
  services_init(ptr_init_data);
  advertising_init();
  conn_params_init();
  peer_manager_init();
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
        /* Advertising is started by PM_EVT_PEERS_DELETED_SUCEEDED event */
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

    /*err_code = bsp_indication_set(BSP_INDICATE_IDLE);*/
    /*APP_ERROR_CHECK(err_code);*/

    /* Prepare wakeup buttons. */
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    /* Go to system-off mode (this function will not return; wakeup will cause a reset). */
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/*******************************************************************************
 *
 *		PRIVATE FUNCTIONS SECTION
 *
 ******************************************************************************/

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
static void services_init(espresso_user_config_t* ptr_init_data)
{
  ret_code_t         err_code;
  nrf_ble_qwr_init_t qwr_init = {0};

  /* Initialize Queued Write Module. */
  qwr_init.error_handler = nrf_qwr_error_handler;
  err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
  APP_ERROR_CHECK(err_code);

  /* Initialize CUS for BLE ESPRESSO APPLICATION */
  ble_cus_init_t    cus_init = {0};
  /*
  cus_evt_handler is the event handler that will identify which characteristic
  of the service and write the received data to g_Espresso_user_config_s  from phone
  */
  cus_init.evt_handler  = cus_evt_handler;
  /* FCN: ble_cus_init create two service for the app
  - SERVICE FOR BREW
  - SERVICE FOR PID
  */
  err_code = ble_cus_init(&m_cus, &cus_init, ptr_init_data );
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
            /*err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);*/
            /*APP_ERROR_CHECK(err_code);*/
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
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("Disconnected.");
            #endif
            /* LED indication will be changed when advertising starts. */
            break;

        case BLE_GAP_EVT_CONNECTED:
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("Connected.");
            #endif
            /*err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);*/
            /*APP_ERROR_CHECK(err_code);*/
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            #if(NRF_LOG_ENABLED == 1)
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
            /* Disconnect on GATT Client timeout event. */
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_DEBUG("GATT Client Timeout.");
            #endif
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            /* Disconnect on GATT Server timeout event. */
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_DEBUG("GATT Server Timeout.");
            #endif
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            /* No implementation needed. */
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

    /* Configure the BLE stack using the default settings. */
    /* Fetch the start address of the application RAM. */
    uint32_t ram_start = 0U;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    /* Enable BLE stack. */
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    /* Register a handler for BLE events. */
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

    /* Security parameters to be used for all security procedures. */
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
    #if(NRF_LOG_ENABLED == 1)
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
 * Function:    cus_evt_handler
 * Description: Custom-service event handler, runs in SoftDevice IRQ context.
 *              Config write events: parse + range-check the payload through
 *              BLE_CFG_CHAR_arr. Valid -> update g_Espresso_user_config_s,
 *              mark the NVM region pending and restart the save debounce.
 *              Invalid -> keep the old value and restore it in the GATT table.
 *              Never touches SPI/NVM: saving is deferred to the main loop.
 *              Notify enable/disable events: log only.
 *****************************************************************************/
static void cus_evt_handler(ble_cus_t * p_cus, ble_cus_evt_t * p_evt)
{
    const ble_cfg_char_t *ptr_row;
    float value;

    UNUSED_PARAMETER(p_cus);
    if ((p_evt->evt_type >= BLE_CFG_EVT_FIRST) && (p_evt->evt_type <= BLE_CFG_EVT_LAST))
    {
        ptr_row = &BLE_CFG_CHAR_arr[(uint32_t)p_evt->evt_type - (uint32_t)BLE_CFG_EVT_FIRST];
        /* All param_command members are struct_CharData at the same address */
        if (parse_ble_ascii_float(&p_evt->param_command.Brew_temp_s, ptr_row->intDigits,
                                  ptr_row->minVal, ptr_row->maxVal, &value) == true)
        {
            *ptr_row->ptrValue      = value;
            g_ble_cfg_pending_mask |= ptr_row->pendingBit;
            g_ble_cfg_quiet_secs    = 0U;
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("BLE cfg %d = " NRF_LOG_FLOAT_MARKER, p_evt->evt_type,
                         NRF_LOG_FLOAT(value));
            #endif
        }
        else
        {
            restore_gatt_value(&p_evt->param_command.Brew_temp_s, ptr_row);
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_WARNING("BLE cfg %d rejected", p_evt->evt_type);
            #endif
        }
        return;
    }

    switch(p_evt->evt_type)
    {
     /* Event -> Enable notification to get water temperature */
        case BLE_MACHINE_BOILER_TEMP_CHAR_NOTIFY_ENABLED:
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_DEBUG("BLE -> Boiler Water Temp Notifications ENABLE");
            #endif
        break;
    /* Event -> Disable notification to get water temperature */
        case BLE_MACHINE_BOILER_TEMP_CHAR_NOTIFY_DISABLED:
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_DEBUG("BLE -> Boiler Water Temp Notifications DISABLE");
            #endif
        break;
    /* Event -> Enable notification to get machine status */
        case BLE_MACHINE_STATUS_CHAR_NOTIFY_ENABLED:
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_DEBUG("BLE -> Blespresso Status Notifications ENABLE");
            #endif
        break;
    /* Event -> Disbale notification to get machine status */
        case BLE_MACHINE_STATUS_CHAR_NOTIFY_DISABLED:
            #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_DEBUG("BLE -> Blespresso Status Notifications DISABLE");
            #endif
        break;
        default:
            /* CONNECTED / DISCONNECTED: no implementation needed. */
            break;
    }
}

/*****************************************************************************
 * Function:    parse_ble_ascii_float
 * Description: Converts a fixed-width ASCII payload from the mobile app
 *              ("DDd" or "DDDd", last digit = 1 decimal) into a float.
 *              Rejects the payload if length != int_digits+1, if any byte is
 *              not '0'..'9', or if the value is outside [min_val, max_val].
 *              Pure check: never changes the user config.
 * Return:      true  -> *ptr_out holds the valid value
 *              false -> payload rejected, *ptr_out untouched
 *****************************************************************************/
static bool parse_ble_ascii_float(const struct_CharData *ptr_char, uint8_t int_digits,
                                  float min_val, float max_val, float *ptr_out)
{
    uint16_t idx;
    float value;

    if (ptr_char->length != (uint16_t)(int_digits + 1U))
    {
        return false;
    }
    for (idx = 0U; idx < ptr_char->length; idx++)
    {
        if ((ptr_char->ptr_data[idx] < (uint8_t)'0') || (ptr_char->ptr_data[idx] > (uint8_t)'9'))
        {
            return false;
        }
    }
    value = chr_array_to_float((char *)ptr_char->ptr_data, (char)int_digits, 1);
    if ((value < min_val) || (value > max_val))
    {
        return false;
    }
    *ptr_out = value;
    return true;
}

/*****************************************************************************
 * Function:    restore_gatt_value
 * Description: After a rejected write, puts the current (last valid) value
 *              back into the characteristic's GATT attribute, so a later
 *              read from the app shows the real value instead of the
 *              rejected payload.
 *****************************************************************************/
static void restore_gatt_value(const struct_CharData *ptr_char, const ble_cfg_char_t *ptr_row)
{
    uint8_t value_arr[BLE_CFG_PAYLOAD_MAX];
    ble_gatts_value_t gatts_value;

    float_to_chr_array(*ptr_row->ptrValue, value_arr, (char)ptr_row->intDigits, 1);
    gatts_value.len     = (uint16_t)(ptr_row->intDigits + 1U);
    gatts_value.offset  = 0U;
    gatts_value.p_value = value_arr;
    (void)sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, ptr_char->handle, &gatts_value);
}

/*****************************************************************************
* Function: 	ble_notify_boiler_water_temp
* Description:  Send data to Mobile as NOTIFICATION
* Caveats:      Youtube-TimeStamp: 2:10:00  - 2:26:00
*****************************************************************************/
void ble_notify_boiler_water_temp(float waterTemp)
{
      ret_code_t err_code;
      uint32_t intTemperature = (uint32_t)(waterTemp * 10.0f);
      uint8_t sTemp[4] = {0};
      uint8_t sbleTemp[4] = {'0','0','0','0'};
      sprintf((char*)sTemp, "%d", intTemperature);
      uint8_t len;
      len = strlen((char*)sTemp);
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
      err_code = ble_cus_notify_boiler_water_temp(&m_cus, sbleTemp, m_conn_handle);
}
