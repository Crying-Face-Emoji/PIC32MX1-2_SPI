/* 
 * File:   PIC32mx_SysTick.cpp
 * Author: Rafael Silva
 * 
 * Created on May 27, 2018, 7:02 PM
 */

#include "SysTick_pic32mx.h"

volatile uint32_t SysTick::count = 0;
volatile uint32_t SysTick::timeout = 0;
volatile bool SysTick::timeoutFlag = 0;

extern "C"
{
	void __ISR(_CORE_TIMER_VECTOR, IPL7AUTO) handler(void)
	{
		uint32_t period = 0;
		// get the old compare time
		asm volatile("mfc0   %0, $11" : "=r"(period));
		period += (GetSystemClock()/2000);
		// set up the period in the compare reg
		asm volatile("mtc0   %0,$11" : "+r"(period));

		SysTick::timeoutFlag = (SysTick::count == SysTick::timeout);

		SysTick::count++;

		IFS0CLR = 0x00000001;
	}
}

void SysTick::init(void)
{
    uint32_t period = 0;
    
//  IFS0<0> IEC0<0> IPC0<4:2> IPC0<1:0>
    IPC0SET = 0x0000001F;
    IEC0SET = 0x00000001;
    IFS0CLR = 0x00000001;
 
    period = (GetSystemClock()/2000);
    
//    void OpenCoreTimer(unsigned int period)
    asm volatile("mtc0   $0,$9");
//  set up the period in the compare reg
    asm volatile("mtc0   %0,$11" : "+r"(period));
}