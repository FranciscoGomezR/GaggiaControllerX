/*
 * StateMachineControls.h
 *
 *  Created on: Aug 1, 2016
 *      Author: JuanFco
 */

#ifndef X01_STATEMACHINECONTROLS_H_
#define X01_STATEMACHINECONTROLS_H_

#include <stdint.h>

#define STATE_MACHINE_RUNNING	0
#define STATE_MACHINE_IDLE		1

typedef struct
	{
		uint8_t sPrevious;
		uint8_t sRunning;
		uint8_t sNext;
	}StateMachineCtrl_Struct;


#endif /* UTILITIES_STATEMACHINECONTROLS_H_ */
