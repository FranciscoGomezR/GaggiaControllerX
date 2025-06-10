
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
#ifndef STORAGECONTROLLER_H__
#define STORAGECONTROLLER_H__
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf.h"
#include "nrf_soc.h"
#include "nordic_common.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "BLEspressoServices.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
  #define PARAM_NVM_START_ADDR    0x3e000
  #define PARAM_NVM_END_ADDR      0x3ffff

  //#define PARAM_NVM_START_ADDR    0x7e000
  //#define PARAM_NVM_END_ADDR      0x80000

  #define PARAM_NVM_MEM_KEY       0x00aa00aa

  //https://devzone.nordicsemi.com/f/nordic-q-a/31017/fstorage-vs-softdevice-activity-application-halts-when-writing-to-flash-if-waiting-to-write-or-data-not-correctly-written-if-not-waiting
  //https://devzone.nordicsemi.com/f/nordic-q-a/22325/flash-data-storage-place-in-memory
  //https://docs.nordicsemi.com/bundle/sdk_nrf5_v16.0.0/page/lib_bootloader.html#lib_bootloader_memory
//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************


//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
  void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);


  void fstorage_Init(void);
  void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage);
  void wait_for_flash_ready_noSoftDevice(nrf_fstorage_t const * p_fstorage);

  void fcn_WriteParameterNVM(BLEspressoVariable_struct * ptr_writeParam);
  void fcn_Read_ParameterNVM(BLEspressoVariable_struct * ptr_ReadParam);

#endif // BLESPRESSOSERVICES_H__