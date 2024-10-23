
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

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************


//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************



//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
  

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
  static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

  //nrf_fstorage_t
  NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
    {
        /* Set a handler for fstorage events. */
        .evt_handler = fstorage_evt_handler,

        /* These below are the boundaries of the flash space assigned to this instance of fstorage.
         * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
         * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
         * last page of flash available to write data. */
        .start_addr = 0x3e000,
        .end_addr   = 0x3ffff,
    };

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
  {
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
                case NRF_FSTORAGE_EVT_READ_RESULT:
        {
            NRF_LOG_INFO("--> Event received: read %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
  }

  static uint32_t nrf5_flash_end_addr_get();
  void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage);

  void fcn_runtest(void);



#endif // BLESPRESSOSERVICES_H__