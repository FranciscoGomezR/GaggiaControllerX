
/*************************************************************************************
* 	Revision History:
*
*   Date          	CP#           Author
*   DD-MM-YYYY      XXXXX:1		Initials	Description of change
*   -----------   ------------  ---------   ------------------------------------
*  	XX-XX-XXXX		X.X			ABCD		"CHANGE"	
*
*************************************************************************************
*
* File/

*  "More detail description of the code"
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
*  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
*  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*  DEALINGS IN THE SOFTWARE.
*
*/
#ifndef BLESPRESSOSERVICES_H__
#define BLESPRESSOSERVICES_H__
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "bluetooth_drv.h"
#include <stdbool.h>
#include <stdint.h>

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
#define BLE_SERVICE_BLEESPRESSO_UUID            0x1400

#define BLE_CHAR_BLESPRESSO_STATUS__UUID        0x1401

#define BLE_CHAR_BOILER_WATER__TEMP_UUID        0x1402
#define BLE_CHAR_BOILER_TARGET_TEMP_UUID        0x1403

#define BLE_CHAR_BREW_PRE_INFUSION_POWER_UUID   0x1404
#define BLE_CHAR_BREW_PRE_INFUSION_TIME__UUID   0x1405
#define BLE_CHAR_BREW_INFUSION_POWER_UUID       0x1406
#define BLE_CHAR_BREW_INFUSION_TIME__UUID       0x1407
#define BLE_CHAR_BREW_DECLINING_PR_POWER_UUID   0x1408
#define BLE_CHAR_BREW_DECLINING_PR_TIME__UUID   0x1409

#define BLE_SERVICE_PIDESPRESSO_UUID            0x1500

#define BLE_CHAR_PID_P_TERM_UUID                0x1501
#define BLE_CHAR_PID_I_TERM_UUID                0x1502
#define BLE_CHAR_PID_I_MAX_TERM_UUID            0x1503
#define BLE_CHAR_PID_I_TERM_WINDUP_UUID         0x1504
#define BLE_CHAR_PID_D_TERM_UUID                0x1505
#define BLE_CHAR_PID_D_TERM_LPF_UUID            0x1506
#define BLE_CHAR_PID_GAIN___UUID                0x1507

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
  typedef struct BLEspressoVariable_struct{
    uint32_t nvmBuffer;
    uint32_t nvmKey;
    float TargetBoilerTemp;
    float ActualBoilerTemp;
    float BrewPreInfussionPwr;
    float BrewPreInfussionTmr;
    float BrewInfussionPwr;
    float BrewInfussionTmr;
    float BrewDecliningPwr;
    float BrewDecliningTmr;

    float Pid_P_term;
    float Pid_I_term;
    float Pid_Imax_term;
    bool  Pid_Iwindup_term;
    float Pid_D_term;
    float Pid_Dlpf_term;
    float Pid_Gain_term;
   }BLEspressoVariable_struct;

   //17 floats
//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
 extern volatile BLEspressoVariable_struct BLEspressoVar;
 extern volatile BLEspressoVariable_struct int_NvmData;
  

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************


#endif // BLESPRESSOSERVICES_H__