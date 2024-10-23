/**
 * Copyright (c) 2017 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */



#ifndef APP_CONFIG_H
#define APP_CONFIG_H
// <<< Use Configuration Wizard in Context Menu >>>\n

#define inBREW_PIN          27
#define inSTEAM_PIN         28
#define inZEROCROSS_PIN     26
#define outSSRboiler_PIN    19
#define outSSRpump_PIN      20
#define enSelenoidRelay_PIN 25
#define enDC12Voutput_PIN   3

#define outSTATE__ON        1
#define outSTATE_OFF        0


#define TWI_ENABLED 1
// <o> TWI_DEFAULT_CONFIG_FREQUENCY  - Frequency
 
// <26738688=> 100k 
// <67108864=> 250k 
// <104857600=> 400k 
#define TWI_DEFAULT_CONFIG_FREQUENCY 26738688
#define TWI0_ENABLED 1
#define TWI0_USE_EASY_DMA 1

#define NRFX_TWIM_ENABLED 1
#define NRFX_TWIM_DEFAULT_CONFIG_FREQUENCY 26738688

#define NRFX_TWI_ENABLED 1
#define NRFX_TWI_DEFAULT_CONFIG_FREQUENCY 26738688

//interface between NRF -> MAX31865
#define SPI_SCK_PIN   13
#define SPI_MISO_PIN  11
#define SPI_MOSI_PIN  14
#define SPI_SS_PIN    12

#define NRFX_GPIOTE_ENABLED 1
#define NRFX_PRS_ENABLED 1

#define SPI_ENABLED 1
#define SPI1_ENABLED 1
#define SPI1_USE_EASY_DMA 1

#define NRFX_SPIM_ENABLED 1
#define NRFX_SPIM1_ENABLED 1
#define NRFX_SPI_ENABLED 1

#define BLE_NUS_ENABLED 1
#define NRF_SDH_BLE_VS_UUID_COUNT 2  


// <<< end of configuration section >>>
#endif //APP_CONFIG_H

