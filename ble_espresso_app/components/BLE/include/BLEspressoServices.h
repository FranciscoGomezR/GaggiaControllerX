
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
#define LOAD_USERDATA_FROM_NVM_EN     1
#define STORE_USERDATA_TO_NVM_EN      1

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
  typedef struct bleSpressoUserdata_struct{
    uint32_t nvmWcycles;
    uint32_t nvmKey;
    float temp_Target;
    float temp_Boiler;

    float prof_preInfusePwr;
    float prof_preInfuseTmr;
    float prof_InfusePwr;
    float prof_InfuseTmr;
    float Prof_DeclinePwr;
    float Prof_DeclineTmr;

    float Pid_P_term;
    float Pid_I_term;
    float Pid_Imax_term;
    bool  Pid_Iwindup_term;
    float Pid_D_term;
    float Pid_Dlpf_term;
    float Pid_Gain_term;
   }bleSpressoUserdata_struct;

   //14 floats
   //2 uint32_t
   //1  byte
   //Equal to 65 bytes
//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
extern volatile bleSpressoUserdata_struct blEspressoProfile;
extern volatile bleSpressoUserdata_struct int_NvmData;
 
#if STORE_USERDATA_TO_NVM_EN == 1
  extern volatile bleSpressoUserdata_struct testProfileData;
#endif

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************


#endif // BLESPRESSOSERVICES_H__