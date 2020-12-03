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

#include "../include/buttons.h"
#include "../include/nixie.h"

// Button variables
volatile uint8_t set_button_counter = 0x00;
volatile uint8_t adv_button_counter = 0x00;
volatile uint8_t set_button_flag, adv_button_flag;
volatile uint8_t set_timer = 0xff;
volatile uint8_t set_button_holdoff = 0x00;

// Timer 0 functions (button/event timer)
ISR (TIMER0_OVF_vect) {
	// test 'set' button
	if (!(SET_BUTTON_PORT & (1 << SET_BUTTON_IDX))) {
		set_button_counter++;
	} else {
		set_button_counter = 0x00;
		set_button_flag = NOT_PRESSED;
	}
	
	// set flags for the button state
	if (set_button_counter >= CONT_PRESS_TIMEOUT) {
		set_button_flag = CONTINUED_PRESS;
		set_button_counter = LONG_PRESS_TIMEOUT + 1;
	} else if (set_button_counter == LONG_PRESS_TIMEOUT) {
		set_button_flag = LONG_PRESS;
	} else if (set_button_counter == SHORT_PRESS_TIMEOUT) {
		set_button_flag = SHORT_PRESS;
	}

	// test 'adv' button
	if (!(ADV_BUTTON_PORT & (1 << ADV_BUTTON_IDX))) {
		adv_button_counter++;
	} else {
		adv_button_counter = 0x00;
		adv_button_flag = NOT_PRESSED;
	}
	
	// set flags for the button state
	if (adv_button_counter >= CONT_PRESS_TIMEOUT) {
		adv_button_flag = CONTINUED_PRESS;
		adv_button_counter = LONG_PRESS_TIMEOUT + 1;
	} else if (adv_button_counter == LONG_PRESS_TIMEOUT) {
		// long press
		adv_button_flag = LONG_PRESS;
	} else if (adv_button_counter == SHORT_PRESS_TIMEOUT) {
		// short press
		adv_button_flag = SHORT_PRESS;
	}	
}

ISR (TIMER0_COMP_vect) {
// Nothing to see here
}

void init_event_timer(void) {
	TCNT0 = 0x00;
	TCCR0 = (1 << CS02) | (1 << CS00);			// Timer mode with 1024 prescaler 8MHz / 1024 = 7.8125KHz step / 256 steps = 30.5Hz overflow ~32.7ms
	//TIMSK |= (1 << TOIE0) | (1 << OCIE0);		// Enable timer1 overflow and compare interrupt
	TIMSK |= (1 << TOIE0);						// Enable timer overflow interrupt
	sei();   
}

