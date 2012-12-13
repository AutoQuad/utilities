/*
    This file is part of AutoQuad.

    AutoQuad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AutoQuad is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with AutoQuad.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2011, 2012  Bill Nesbitt
*/

#ifndef _supervisor_h
#define _supervisor_h

#include <CoOS.h>
#include "digital.h"

#define SUPERVISOR_STACK_SIZE	    72
#define SUPERVISOR_PRIORITY	    34

#define SUPERVISOR_RATE		    10		    // Hz
#define SUPERVISOR_DISARM_TIME	    (1e6*2)	    // 2 seconds
#define SUPERVISOR_RADIO_LOSS1	    ((int)1e6)	    // 1 second
#define SUPERVISOR_RADIO_LOSS2	    ((int)15e6)	    // 15 seconds

#define SUPERVISOR_SOC_TABLE_SIZE   100

enum supervisorStates {
    STATE_INITIALIZING	= 0x00,
    STATE_DISARMED	= 0x01,
    STATE_ARMED		= 0x02,
    STATE_FLYING	= 0x04,
    STATE_RADIO_LOSS1	= 0x08,
    STATE_RADIO_LOSS2	= 0x10,
    STATE_LOW_BATTERY1	= 0x20,
    STATE_LOW_BATTERY2	= 0x40
};

typedef struct {
    OS_TID supervisorTask;

    digitalPin *readyLed;
    digitalPin *debugLed;

    float socTable[SUPERVISOR_SOC_TABLE_SIZE+1];
    float soc;
    float flightTime;		    // seconds
    float flightSecondsAvg;	    // avg flight time seconds for every percentage of SOC
    float flightTimeRemaining;	    // seconds
    uint32_t armTime;
    uint32_t lastGoodRadioMicros;
    float vInLPF;
    uint8_t state;
    uint8_t diskWait;
    uint8_t configRead;
} supervisorStruct_t;

extern supervisorStruct_t supervisorData;

extern void supervisorInit(void);
extern void supervisorInitComplete(void);
extern void supervisorDiskWait(uint8_t waiting);
extern void supervisorThrottleUp(uint8_t throttle);
extern void supervisorSendDataStart(void);
extern void supervisorSendDataStop(void);
extern void supervisorConfigRead(void);
extern void supervisorArm(void);
extern void supervisorDisarm(void);

#endif
