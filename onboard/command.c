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

#include "aq.h"
#include "command.h"
#include "serial.h"
#include "telemetry.h"
#include "downlink.h"
#include "notice.h"
#include "control.h"
#include "config.h"
#include "gps.h"
#include "adc.h"
#include "aq_mavlink.h"
#include "util.h"
#include "imu.h"
#include "supervisor.h"
#include <CoOS.h>
#include <string.h>

commandStruct_t commandData __attribute__((section(".ccm")));

OS_STK *commandTaskStack;

void commandChecksum(unsigned char c) {
    commandData.checkA += c;
    commandData.checkB += commandData.checkA;
}

void commandResponseSend(commandBufStruct_t *r, unsigned char len) {
    char *c = (char *)r;

    // grab port lock
    CoEnterMutexSection(downlinkData.serialPortMutex);

    downlinkSendString("AqR");	// response header

    downlinkResetChecksum();
    downlinkSendChar(commandData.requestId[0]);
    downlinkSendChar(commandData.requestId[1]);
    downlinkSendChar(commandData.requestId[2]);
    downlinkSendChar(commandData.requestId[3]);
    downlinkSendChar(len);
    do {
	downlinkSendChar(*c++);
    } while (--len);

    downlinkSendChecksum();

    // release serial port
    CoLeaveMutexSection(downlinkData.serialPortMutex);
}

void commandRespond(unsigned char c, char *reply, unsigned char len) {
    commandBufStruct_t resp;

    resp.commandId = c;
    if (reply) {
	memcpy((char *)&resp.data, reply, len);
	commandResponseSend(&resp, len + sizeof(resp.commandId));
    }
    else {
	commandResponseSend(&resp, sizeof(resp.commandId));
    }
}

void commandAck(char *reply, unsigned char len) {
    commandRespond(COMMAND_ID_ACK, reply, len);
}

void commandNack(char *reply, unsigned char len) {
    commandRespond(COMMAND_ID_NACK, reply, len);
}

// temporary to receive external acc from serial port
void commandAccIn(commandBufStruct_t *b) {
    typedef struct {
	unsigned char command;
	unsigned char rows;
	unsigned char floats;
	float values[10];
    } __attribute__((packed)) dumpStruct_t;
    dumpStruct_t *dump;

    dump = (dumpStruct_t *)b;

    // reverse rotation
    commandData.floatDump[0] = dump->values[0];
    commandData.floatDump[1] = dump->values[1];
    commandData.floatDump[2] = dump->values[2];
    commandData.floatDump[3] = dump->values[3];
    commandData.floatDump[4] = dump->values[4];
    commandData.floatDump[5] = dump->values[5];
    commandData.floatDump[6] = dump->values[6];
    commandData.floatDump[7] = dump->values[7];
    commandData.floatDump[8] = dump->values[8];
    commandData.floatDump[9] = dump->values[9];
}

void commandExecute(commandBufStruct_t *b) {
    switch (b->commandId) {
    case COMMAND_GPS_PACKET:
	    // first character is length
	    gpsSendPacket(*b->data, b->data+1);
	    commandAck(NULL, 0);
	    break;

    case COMMAND_ID_GPS_PASSTHROUGH:
	    if (!(supervisorData.state & STATE_FLYING)) {
		    telemetryDisable();
		    commandAck(NULL, 0);
		    gpsPassthrough(gpsData.gpsPort, downlinkData.serialPort);
	    }
	    else {
		    commandNack(NULL, 0);
	    }
	    break;
    case COMMAND_CONFIG_PARAM_READ:
	    {
		    unsigned int replyLength = configParameterRead(b->data);

		    if (replyLength)
			    commandAck(b->data, replyLength+8);
		    else
			    commandNack(NULL, 0);
	    }
	    break;

    case COMMAND_CONFIG_PARAM_WRITE:
	    if (!(supervisorData.state & STATE_FLYING)) {
		    unsigned int replyLength = configParameterWrite(b->data);

		    if (replyLength)
			    commandAck(b->data, replyLength);
		    else
			    commandNack(NULL, 0);
	    }
	    else {
		    commandNack(NULL, 0);
	    }
	    break;

    case COMMAND_CONFIG_FLASH_READ:
	    if (!(supervisorData.state & STATE_FLYING)) {
		    configFlashRead();
		    commandAck(NULL, 0);
	    }
	    else {
		    commandNack(NULL, 0);
	    }
	    break;

    case COMMAND_CONFIG_FLASH_WRITE:
	    if (!(supervisorData.state & STATE_FLYING)) {
		    if (configFlashWrite())
			    commandAck(NULL, 0);
		    else
			    commandNack(NULL, 0);
	    }
	    else {
		    commandNack(NULL, 0);
	    }
	    break;

    case COMMAND_CONFIG_FACTORY_RESET:
	    if (!(supervisorData.state & STATE_FLYING)) {
		    configLoadDefault();
		    commandAck(NULL, 0);
	    }
	    else {
		    commandNack(NULL, 0);
	    }
	    break;

    case COMMAND_TELEMETRY_DISABLE:
	    telemetryDisable();
	    commandAck(NULL, 0);
	    break;

    case COMMAND_TELEMETRY_ENABLE:
	    telemetryEnable();
	    commandAck(NULL, 0);
	    break;

    case COMMAND_ACC_IN:
	    commandAccIn(b);
	    break;

#ifdef IMU_HAS_VN100
    case COMMAND_VN100_PASSTHROUGH:
	    if (!(supervisorData.state & STATE_FLYING))
		vn100PassThrough(downlinkData.serialPort);
	    break;

    case COMMAND_VN100_BOOTLOADER:
	    if (!(supervisorData.state & STATE_FLYING))
		vn100BootLoader(downlinkData.serialPort);
	    break;

    case COMMAND_VN100_READ_CONFIG:
	    if (!(supervisorData.state & STATE_FLYING) && vn100ReadConfig() == 0)
		commandAck(NULL, 0);
	    else
		commandNack(NULL, 0);

	    break;

    case COMMAND_VN100_WRITE_CONFIG:
	    if (!(supervisorData.state & STATE_FLYING) && vn100WriteConfig() == 0)
		commandAck(NULL, 0);
	    else
		commandNack(NULL, 0);
	    break;
#endif

    default:
	    commandNack(NULL, 0);
	    break;
    }
}

void commandCharIn(unsigned char ch) {
    switch (commandData.state) {
    case COMMAND_WAIT_SYNC1:
	if (ch == COMMAND_SYNC1)
	    commandData.state = COMMAND_WAIT_SYNC2;
	break;

    case COMMAND_WAIT_SYNC2:
	if (ch == COMMAND_SYNC2)
	    commandData.state = COMMAND_WAIT_SYNC3;
	else
	    commandData.state = COMMAND_WAIT_SYNC1;
	break;

    case COMMAND_WAIT_SYNC3:
	if (ch == COMMAND_COMMAND) {
	    commandData.checkA = commandData.checkB = 0;
	    commandData.state = COMMAND_WAIT_RQID1;
	}
	else if (ch == COMMAND_FLOAT_DUMP) {
	    commandData.buf.commandId = COMMAND_ACC_IN;
	    commandData.bufPoint = 1;
	    commandData.checkA = commandData.checkB = 0;
	    commandData.state = COMMAND_WAIT_ROWS;
	}
	else
	    commandData.state = COMMAND_WAIT_SYNC1;
	break;

    case COMMAND_WAIT_RQID1:
	commandChecksum(ch);
	commandData.requestId[0] = ch;
	commandData.state = COMMAND_WAIT_RQID2;
	break;

    case COMMAND_WAIT_RQID2:
	commandChecksum(ch);
	commandData.requestId[1] = ch;
	commandData.state = COMMAND_WAIT_RQID3;
	break;

    case COMMAND_WAIT_RQID3:
	commandChecksum(ch);
	commandData.requestId[2] = ch;
	commandData.state = COMMAND_WAIT_RQID4;
	break;

    case COMMAND_WAIT_RQID4:
	commandChecksum(ch);
	commandData.requestId[3] = ch;
	commandData.state = COMMAND_WAIT_LEN;
	break;

    case COMMAND_WAIT_LEN:
	commandChecksum(ch);
	commandData.len = ch;
	commandData.bufPoint = 0;
	commandData.state = COMMAND_PAYLOAD;
	break;

    case COMMAND_WAIT_ROWS:
	commandChecksum(ch);
	*((char *)&commandData.buf + commandData.bufPoint++) = ch;
	commandData.len = ch;
	commandData.state = COMMAND_WAIT_FLOATS;
	break;

    case COMMAND_WAIT_FLOATS:
	commandChecksum(ch);
	*((char *)&commandData.buf + commandData.bufPoint++) = ch;
	commandData.len = commandData.bufPoint + commandData.len * ch * sizeof(float);
	commandData.state = COMMAND_PAYLOAD;
	break;

    case COMMAND_PAYLOAD:
	commandChecksum(ch);
	*((char *)&commandData.buf + commandData.bufPoint++) = ch;
	if (commandData.bufPoint == commandData.len)
	    commandData.state = COMMAND_CHECK1;
	break;

    case COMMAND_CHECK1:
	if (ch == commandData.checkA) {
	    commandData.state = COMMAND_CHECK2;
	}
	else {
	    commandData.state = COMMAND_WAIT_SYNC1;
	    commandData.checksumErrors++;
	}
	break;

    case COMMAND_CHECK2:
	commandData.state = COMMAND_WAIT_SYNC1;
	if (ch == commandData.checkB)
	    commandExecute(&commandData.buf);
	else
	    commandData.checksumErrors++;
	break;
    }
}

void commandTaskCode(void *unused) {
    serialPort_t *s = downlinkData.serialPort;

    AQ_NOTICE("Command task started\n");

    while (1) {
	// wait for data
	yield(5);

	// process incoming data
	while (serialAvailable(s))
	    commandCharIn(downlinkReadChar());
    }
}

void commandInit(void) {
    AQ_NOTICE("Command interface init\n");

    memset((void *)&commandData, 0, sizeof(commandData));

    commandTaskStack = aqStackInit(COMMAND_STACK_SIZE, "COMMAND");

    commandData.commandTask = CoCreateTask(commandTaskCode, (void *)0, COMMAND_PRIORITY, &commandTaskStack[COMMAND_STACK_SIZE-1], COMMAND_STACK_SIZE);

    commandData.state = COMMAND_WAIT_SYNC1;
}
