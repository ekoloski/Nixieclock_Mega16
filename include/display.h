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

#ifndef __DISPLAY_H_
#define __DISPLAY

#include <avr/io.h>

//Pin definitions:
//Hours Tens:
#define HOUR_TEN_PORT	PORTA
#define HOUR_TEN_MASK	0b00001111
#define hour_ten_a		PA3
#define hour_ten_b		PA1
#define hour_ten_c		PA0
#define hour_ten_d		PA2
//Hours Ones:
#define HOUR_ONE_PORT	PORTA
#define HOUR_ONE_MASK	0b11110000
#define hour_one_a		PA7
#define hour_one_b		PA5
#define hour_one_c		PA4	
#define hour_one_d		PA6
//Minutes Tens:
#define MIN_TEN_PORT	PORTB
#define MIN_TEN_MASK	0b00011110
#define min_ten_a		PB4
#define min_ten_b		PB2
#define min_ten_c		PB1
#define min_ten_d		PB3
//Minutes Ones:
#define MIN_ONE_PORT	PORTB
#define MIN_ONE_MASK	0b11100000
#define min_one_a		PD0
#define min_one_b		PB6
#define min_one_c		PB5
#define min_one_d		PB7
//Seconds Tens:
#define SEC_TEN_PORT	PORTD
#define SEC_TEN_MASK	0b01111000	
#define sec_ten_a		PD6	
#define sec_ten_b		PD4	
#define sec_ten_c		PD3	
#define sec_ten_d		PD5
//Seconds Ones:
#define SEC_ONE_PORT	PORTC
#define SEC_ONE_MASK	0b00011110
#define sec_one_a		PC1
#define sec_one_b		PC3
#define sec_one_c		PC4
#define sec_one_d		PC2
// Colons
#define RCOL_MASK		0b00000010
#define LCOL_MASK		0b00000001
#define RCOL_PORT		PORTD
#define LCOL_PORT		PORTB
#define rcol_pin		PD1
#define lcol_pin		PB0

// Set mode stuff
#define HOUR_SET		1
#define MIN_SET			2
#define SEC_SET			3
#define MONTH_SET		4
#define DATE_SET		5
#define YEAR_SET		6

extern volatile uint8_t display_state;					// time or date, probably get rid of this...

// Stuff used in fading and whatnot
extern volatile uint8_t override_pwm;					// Force full brightness, for use in menus and set mode
extern volatile uint8_t display_duty_cycle;				// brightness
extern volatile uint8_t display_new[3];					// 0: Hours 1: Minutes 2: Seconds
extern volatile uint8_t display_old[3];					// 0: Hours 1: Minutes 2: Seconds
extern volatile uint8_t display_colons;

void init_pwm_timer(void);
void display_blank_digits(uint8_t digits);
void display_set_digits(uint8_t hour, uint8_t minute, uint8_t second, uint8_t colon);

#endif // __DISPLAY_H_