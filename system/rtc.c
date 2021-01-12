// vim: set tabstop=4 shiftwidth=4 expandtab :
//
// nixietherm-firmware - NixieClock Mega Main Firmware Program
// Copyright (C) 2020 Edward Koloski
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http:#www.gnu.org/licenses/>.

#include <avr/io.h>
#include <avr/interrupt.h>

#include "../include/rtc.h"
#include "../include/nixie.h"
#include "../include/display.h"
#include "../include/buttons.h"

// Global Time of Day Cache
volatile timespec_t clock;
volatile uint8_t sentinal = CLEAR;

// Clock variables
volatile uint8_t set_mode = NORMAL;
volatile uint8_t clock_state;
volatile uint8_t unlock_correction = FALSE;
volatile int8_t correction;
volatile int8_t dst_handled;

ISR(TIMER2_COMP_vect) __attribute__ ((hot));
ISR(TIMER2_COMP_vect) {
	// Fractional second interrupt, 1/4 1/2 and 3/4
	switch (OCR2) {
		case 64:								// 1/4 second interrupt
			OCR2 = 128;							// Set the compare for 1/2 second
			sentinal = QRTR_SECOND;
			break;
		case 128:								// Half second interrupt
			sentinal = HALF_SECOND;
			OCR2 = 192;							// Set the compare for 3/4 second
			break;			
		case 192:								// 3/4 second interrupt
			sentinal = TQRT_SECOND;
			OCR2 = 64;							// Set the compare for 1/4 second	
			break;
	}
}

ISR(TIMER2_OVF_vect) __attribute__ ((hot));
ISR(TIMER2_OVF_vect) {
	// Second increment interrupt	
	sentinal = SECOND;					// Set the sentinel so we dont have to do so much shit in the ISR
	
	// Software time correction
	if (correction_flag > 0) {
		// Add a second
		if (clock.second < 59) {
			clock.second += 1;
			correction_flag = 0;
			correction_counter = 0x0000;
		}
	} else if (correction_flag < 0) {
		// Subtract a second
		if (clock.second > 0) {
			clock.second -= 1;
			correction_flag = 0;
			correction_counter = 0x0000;
		}
	}
	
	if (++clock.second==60)	{			//keep track of time, date, month, and year
		clock.second=0;
		if (++clock.minute==60) {
			clock.minute=0;
			if (++clock.hour==24) {
				clock.hour=0;
				unlock_correction = 1;
				dst_handled = 0;
				if (++clock.day==7) 
					clock.day=0;
				if (++clock.date==32) {
					clock.month++;
					clock.date=1;
				} else if (clock.date==31) {
					if ((clock.month==4) || (clock.month==6) || (clock.month==9) || (clock.month==11)) {
						clock.month++;
						clock.date=1;
					}
				} else if (clock.date==30) {
					if(clock.month==2) {
						clock.month++;
						clock.date=1;
					}
				} else if (clock.date==29) {
					if((clock.month==2) && (not_leap())) {
						clock.month++;
						clock.date=1;
					}
				}
				if (clock.month==13) {
					clock.month=1;
					clock.year++;
				}
			}
		}
	}
}

void init_rtc(void) {
	cli();													// Stop interrupts while we set everything up
	ASSR |= (1<<AS2);										// set Timer/counter0 to be asynchronous from the CPU clock
															//  with a second external clock (32,768kHz)driving it.
	TCNT2 =0;												// Reset timer
	OCR2 = 64;												// Set the compare for 1/4 second
	TCCR2 =(1<<CS20)|(1<<CS22);								// Prescale the timer to be clock source/128 to make it
															//  exactly 1 second for every overflow to occur
	while (ASSR & ((1<<TCN2UB)|(1<<OCR2UB)|(1<<TCR2UB)));	// Wait until TC2 is updated
	TIFR = (1<<TOV2);
	TIMSK |= (1<<TOIE2)|(1<<OCIE2);							// Set 8-bit Timer/Counter2 Overflow and Compare Interrupt Enable
	sei();													// Set the Global Interrupt Enable Bit
	
	clock.hour = 0;
	clock.minute = 0;
	clock.second = 0;
	clock.date = 1;
	clock.day = 0;		//0 = sunday, 6 = saturday
	clock.month = 1;
	clock.year = 20;
	clock.day = calculate_day_of_week();	
	dst_handled = 0;
}

char not_leap(void) {										//check for leap year
	if (!(clock.year%100)) {
		return (char)(clock.year%400);
	} else {
		return (char)(clock.year%4);
	}
}