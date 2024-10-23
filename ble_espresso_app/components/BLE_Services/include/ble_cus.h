#ifndef BLE_CUS_H__
#define BLE_CUS_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "BLEspressoServices.h"


#define BLE_CUS_BLE_OBSERVER_PRIO   2

/**@brief   Macro for defining a ble_hrs instance.
 *
 * @param   _name   Name of the instance.
 * Youtube-TimeStamp: 33:20
 */
#define BLE_CUS_DEF(_name)                                                                          \
static ble_cus_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_CUS_BLE_OBSERVER_PRIO,                                                     \
                     ble_cus_on_ble_evt, &_name)

//static uint16_t mm_conn_handle = BLE_CONN_HANDLE_INVALID;

  //Youtube-TimeStamp: 34:00
// CUSTOM_SERVICE_UUID_BASE f364adc9-b000-4042-ba50-05ca45bf8abc
  #define CUSTOM_SERVICE_UUID_BASE         {0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, \
                                            0x40, 0x42, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00}

  #define CUSTOM_PID_SERVICE_UUID_BASE     {0x22, 0x21, 0x20, 0x19, 0x18, 0x17, 0x16, 0x15, \
                                            0x14, 0x13, 0x12, 0x11, 0x00, 0x00, 0x00, 0x00}

																					
/**@brief Custom Service event type. */
typedef enum
{
    BLESPRESSO_STATUS_CHAR_NOTIFICATION_ENABLED,
    BLESPRESSO_STATUS_CHAR_NOTIFICATION_DISABLED,
    BLE_BOILER_TEMP_CHAR_NOTIFICATION_ENABLED,                             /**< Custom value notification enabled event. */
    BLE_BOILER_TEMP_CHAR_NOTIFICATION_DISABLED,                             /**< Custom value notification disabled event. */
    BLE_BOILER_CHAR_EVT_NEW_TEMPERATURE,  //  CHAR_RX_EVT
    //Evetn type: CHAR_RX_EVT -> this event will occur when client write to this char
    BLE_BREW_PRE_INFUSION_POWER_CHAR_RX_EVT,
    BLE_BREW_PRE_INFUSION_TIME__CHAR_RX_EVT,
    BLE_BREW_INFUSION_POWER_CHAR_RX_EVT,
    BLE_BREW_INFUSION_TIME__CHAR_RX_EVT,
    BLE_BREW_DECLINING_PR_POWER_CHAR_RX_EVT,
    BLE_BREW_DECLINING_PR_TIME__CHAR_RX_EVT,

    PID_P_TERM_CHAR_RX_EVT,
    PID_I_TERM_CHAR_RX_EVT,
    PID_I_TERM_INT_CHAR_RX_EVT,
    PID_I_TERM_WINDUP_CHAR_RX_EVT,
    PID_D_TERM_CHAR_RX_EVT,
    PID_D_TERM_LPF_CHAR_RX_EVT,
    PID_GAIN___CHAR_RX_EVT,

    BLE_CUS_EVT_DISCONNECTED,
    BLE_CUS_EVT_CONNECTED
} ble_cus_evt_type_t;

typedef struct
{
    uint8_t const*  ptr_data;
    uint16_t        length;
}struct_CharData;

/**@brief Custom Service event. */
typedef struct
{
    ble_cus_evt_type_t evt_type;     /**< Type of event. */
    //add strucutre of data (string type)to be pass from mobile to BLe stack Youtube-TimeSTamp: 37:00
    union{
      struct_CharData sBoilerTempTarget;
      struct_CharData sBrewPreInfussionPwr;
      struct_CharData sBrewPreInfussionTmr;
      struct_CharData sBrewInfussionPwr;
      struct_CharData sBrewInfussionTmr;
      struct_CharData sBrewDecliningPwr;
      struct_CharData sBrewDecliningTmr;

      struct_CharData sPid_P_term;
      struct_CharData sPid_I_term;
      struct_CharData sPid_Imax_term;
      struct_CharData sPid_Iwindup_term;
      struct_CharData sPid_D_term;
      struct_CharData sPid_Dlpf_term;
      struct_CharData sPid_Gain_term;
    }param_command;
} ble_cus_evt_t;

// Forward declaration of the ble_cus_t type.
typedef struct ble_cus_s ble_cus_t;


/**@brief Custom Service event handler type. */
typedef void (*ble_cus_evt_handler_t) (ble_cus_t * p_bas, ble_cus_evt_t * p_evt);

/**@brief Battery Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
    ble_cus_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
    uint8_t                       initial_custom_value;           /**< Initial custom value */
     
    ble_srv_cccd_security_mode_t  blespressoStatus_char_attr_md; 
    ble_srv_cccd_security_mode_t  boilerTemp_char_attr_md;     /**< Initial security level for Custom characteristics attribute -> For char with notification */
    ble_srv_cccd_security_mode_t  boilerTargetTemp_char_attr_md;

    ble_srv_cccd_security_mode_t  brewPreInfussionPower_char_attr;
    ble_srv_cccd_security_mode_t  brewPreInfussiontime__char_attr;
    ble_srv_cccd_security_mode_t  brewInfussionPower_char_attr;
    ble_srv_cccd_security_mode_t  brewInfussiontime__char_attr;
    ble_srv_cccd_security_mode_t  brewDecliningPower_char_attr;
    ble_srv_cccd_security_mode_t  brewDecliningTime__char_attr;

    ble_srv_cccd_security_mode_t  pid_Pterm_char_attr;
    ble_srv_cccd_security_mode_t  pid_Iterm_char_attr;
    ble_srv_cccd_security_mode_t  pid_ImaxTerm_char_attr;
    ble_srv_cccd_security_mode_t  pid_Iwindup_char_attr;
    ble_srv_cccd_security_mode_t  pid_Dterm_char_attr;
    ble_srv_cccd_security_mode_t  pid_DlpfTerm_char_attr;
    ble_srv_cccd_security_mode_t  pid_GainTerm_char_attr;

} ble_cus_init_t;

/**@brief Custom Service structure. This contains various status information for the service. */
struct ble_cus_s
{
    ble_cus_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
    uint16_t                      service_handle;                 /**< Handle of Custom Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t      blespressoStatus_char_handles;
    ble_gatts_char_handles_t      boilerTemp_char_handles;           /**< Handles related to the Custom Value characteristic. */
    ble_gatts_char_handles_t      boilerTargetTemp_char_handles;

    ble_gatts_char_handles_t      brewPreInfussionPower_char_handles;
    ble_gatts_char_handles_t      brewPreInfussiontime__char_handles;
    ble_gatts_char_handles_t      brewInfussionPower_char_handles;
    ble_gatts_char_handles_t      brewInfussiontime__char_handles;
    ble_gatts_char_handles_t      brewDecliningPower_char_handles;
    ble_gatts_char_handles_t      brewDecliningtime__char_handles;

    ble_gatts_char_handles_t      pid_Pterm_char_handles;
    ble_gatts_char_handles_t      pid_Iterm_char_handles;
    ble_gatts_char_handles_t      pid_ImaxTerm_char_handles;
    ble_gatts_char_handles_t      pid_Iwindup_char_handles;
    ble_gatts_char_handles_t      pid_Dterm_char_handles;
    ble_gatts_char_handles_t      pid_DlpfTerm_char_handles;
    ble_gatts_char_handles_t      pid_GainTerm_char_handles;

    uint16_t                      conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t                       uuid_type; 
};

/**@brief Function for initializing the Custom Service.
 *
 * @param[out]  p_cus       Custom Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_cus_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init);

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Battery Service.
 *
 * @note 
 *
 * @param[in]   p_cus      Custom Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
void ble_cus_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);

/**@brief Function for updating the custom value.
 *
 * @details The application calls this function when the cutom value should be updated. If
 *          notification has been enabled, the custom value characteristic is sent to the client.
 *
 * @note 
 *       
 * @param[in]   p_bas          Custom Service structure.
 * @param[in]   Custom value 
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_cus_BoilerWaterTemperature_update(ble_cus_t * p_cus, uint8_t * ptr_waterTemp, uint16_t conn_handle);

#endif // BLE_CUS_H__
