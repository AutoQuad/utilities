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

#ifndef _timer_h
#define _timer_h

#define TIMER_TIM	TIM5	// can only use 32bit timers TIM2 or TIM5
#define TIMER_CLOCK	(rccClocks.PCLK1_Frequency * 2)
#define TIMER_EN	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE)
#define TIMER_IRQ_CH	TIM5_IRQn
#define TIMER_ISR	TIM5_IRQHandler

#define timerMicros()	TIMER_TIM->CNT

typedef void timerCallback_t(int);

typedef struct {
    timerCallback_t *alarm1Callback, *alarm2Callback, *alarm3Callback;
    int alarm1Parameter, alarm2Parameter, alarm3Parameter;

    uint32_t timerStart;
} timerStruct_t;

extern void timerInit(void);
extern void timerStart(void);
extern uint32_t timerStop(void);
extern void timerCancelAlarm1(void);
extern void timerCancelAlarm2(void);
extern void timerCancelAlarm3(void);
extern void timerSetAlarm1(int32_t us, timerCallback_t *callback, int parameter);
extern void timerSetAlarm2(int32_t us, timerCallback_t *callback, int parameter);
extern void timerSetAlarm3(int32_t us, timerCallback_t *callback, int parameter);

#endif
