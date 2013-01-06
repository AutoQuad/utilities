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

    Copyright � 2011, 2012  Bill Nesbitt
*/

#include "gps.h"
#include "telemetry.h"
#include "serial.h"
#include "digital.h"
#include "nav.h"
#include "notice.h"
#include "aq_timer.h"
#include "config.h"
#include "util.h"
#include "imu.h"
#include "aq_mavlink.h"
#include "filer.h"
#include <CoOS.h>
#include <string.h>

gpsStruct_t gpsData __attribute__((section(".ccm")));

OS_STK *gpsTaskStack;

#ifdef GPS_LOG_BUF
char gpsLog[GPS_LOG_BUF];
#endif

void gpsInitGps(void) {
    ubloxInitGps();
}

void gpsSendSetup(void) {
    ubloxSendSetup();
}

void gpsCheckBaud(serialPort_t *s) {
    if ((IMU_LASTUPD - gpsData.lastMessage) > 10000000) {
	if (!gpsData.baudCycle[++gpsData.baudSlot])
	    gpsData.baudSlot = 0;
	AQ_NOTICE("GPS: trying new baud rate\n");
	serialChangeBaud(s, gpsData.baudCycle[gpsData.baudSlot]);
	ubloxInitGps();
	serialChangeBaud(s, p[GPS_BAUD_RATE]);
	gpsSendSetup();
	gpsData.lastMessage = IMU_LASTUPD;
    }
}

void gpsTaskCode(void *p) {
    serialPort_t *s = gpsData.gpsPort;
    char c;
    char ledOn;
#ifdef GPS_LOG_BUF
    int logPointer = 0;
#endif
    unsigned int ret = 0;

    AQ_NOTICE("GPS task task started\n");

    ubloxInit();

    while (1) {
	yield(10);
	gpsCheckBaud(s);

	ledOn = digitalGet(gpsData.gpsLed);
	if (!ledOn)
	    digitalHi(gpsData.gpsLed);

	while (serialAvailable(s)) {
	    c = serialRead(s);
	    ret = ubloxCharIn(c);

	    // position update
	    if (ret == 1) {
		// notify world of new data
		CoSetFlag(gpsData.gpsPosFlag);
	    }
	    // velocity update
	    else if (ret == 2) {
		// notify world of new data
		CoSetFlag(gpsData.gpsVelFlag);
	    }
	    // lost sync
	    else if (ret == 3) {
		gpsCheckBaud(s);
	    }

#ifdef GPS_LOG_BUF
	    gpsLog[logPointer] = c;
	    logPointer = (logPointer + 1) % GPS_LOG_BUF;
#endif
	}

#ifdef GPS_LOG_BUF
	filerSetHead(gpsData.logHandle, logPointer);
#endif

	if (!ledOn)
	    digitalLo(gpsData.gpsLed);
    }
}

void gpsInit(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;

    AQ_NOTICE("GPS init\n");

    memset((void *)&gpsData, 0, sizeof(gpsData));

    gpsData.baudCycle[0] = p[GPS_BAUD_RATE];
    gpsData.baudCycle[1] = 9600;
    gpsData.baudCycle[2] = 230400;
    gpsData.baudCycle[3] = 115200;
    gpsData.baudCycle[4] = 19200;
    gpsData.baudCycle[5] = 38400;
    gpsData.baudCycle[6] = 0;

    gpsData.baudSlot = 0;

    gpsData.gpsLed = digitalInit(GPS_LED_PORT, GPS_LED_PIN);
    digitalLo(gpsData.gpsLed);

    gpsData.gpsPort = serialOpen(GPS_USART, p[GPS_BAUD_RATE], USART_HardwareFlowControl_None, 512, 512);

    // manual reset flags
    gpsData.gpsVelFlag = CoCreateFlag(0, 0);
    gpsData.gpsPosFlag = CoCreateFlag(0, 0);
    gpsTaskStack = aqStackInit(GPS_STACK_SIZE, "GPS");

    gpsData.gpsTask = CoCreateTask(gpsTaskCode, (void *)0, GPS_PRIORITY, &gpsTaskStack[GPS_STACK_SIZE-1], GPS_STACK_SIZE);

    // External Interrupt line PE1 for timepulse
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource1);

    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // Timepulse interrupt from GPS
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    gpsData.microsPerSecond = AQ_US_PER_SEC<<11;

    gpsData.hAcc = gpsData.vAcc = gpsData.sAcc = 1e6f;

#ifdef GPS_LOG_BUF
    gpsData.logHandle = filerGetHandle(GPS_FNAME);
    filerStream(gpsData.logHandle, gpsLog, GPS_LOG_BUF);
#endif
}

void gpsPassthrough(serialPort_t *s1, serialPort_t *s2) {
    digitalPin *gpsLed;

    gpsLed = digitalInit(GPS_LED_PORT, GPS_LED_PIN);

    AQ_NOTICE("Starting GPS passthrough mode\n");

    // don't allow any preemption
    CoSetPriority(gpsData.gpsTask, 1);

    // this is a dead end
    while (1) {
	if (serialAvailable(s1)) {
	    digitalHi(gpsLed);
	    serialWrite(s2, serialRead(s1));
	    digitalLo(gpsLed);
	}
	if (serialAvailable(s2)) {
	    serialWrite(s1, serialRead(s2));
	}
    }
}

void gpsSendPacket(unsigned char len, char *buf) {
    unsigned int i;

    for (i = 0; i < len; i++)
	serialWrite(gpsData.gpsPort, buf[i]);
}

void EXTI1_IRQHandler() {
    unsigned long tp = timerMicros();
    unsigned long diff = (tp - gpsData.lastTimepulse);

    if (diff > 950000 && diff < 1050000)
	gpsData.microsPerSecond -= (gpsData.microsPerSecond - (signed long)((tp - gpsData.lastTimepulse)<<11))>>5;
    gpsData.lastTimepulse = tp;
    gpsData.TPtowMS = gpsData.lastReceivedTPtowMS;

    EXTI_ClearITPendingBit(EXTI_Line1);
}
