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

// for mega16 fuses should be - lfise:0xE4  hfuse:0x51
// avrdude lfuse    -U lfuse:w:0xE4:m
// avrdude hfuse    -U hfuse:w:0x51:m

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "../include/rtc.h"
#include "../include/nixie.h"
#include "../include/display.h"
#include "../include/buttons.h"

// Settings/config
volatile clock_settings_t clock_settings;
volatile EEMEM clock_settings_t clock_settings_eeprom;
volatile uint8_t menu_option;

// Software time correction stuff
volatile uint32_t correction_counter;			
volatile uint32_t correction_value;
volatile int8_t correction_flag;

// Fancy up/down option for some menu settings
volatile uint8_t inc_dec;
volatile uint8_t swap;

volatile uint8_t just_woke_up = 1;

ISR(INT0_vect) {
// Power Fail pin (PD2) is configured for external interrupt on rising and falling edges.
// Check for power failure and sleep if needed or wake up if needed
	if (!(PF_PORT && (1 << PF_PIN))) {
		// disable interrupts for the the buttons, since the power on confuses them
		TIMSK &= ~(1 << TOIE0);
		
		// Set the ports low, save power
		PORTA = 0x00;
		PORTB = 0x00;
		PORTC = (1<<PC6);
		PORTD = 0x00;
		
		// set sleep mode
		set_sleep_mode (SLEEP_MODE_PWR_SAVE);
		// go to sleep
		sleep_enable();
	} else if (PF_PORT && (1 << PF_PIN)) {
		// wake up!
		sleep_disable ();
		
		// Update the display digits
		display_set_digits(clock.hour, clock.minute, clock.second, display_colons);
		clock_state = NORMAL;
		display_state = NORMAL;		
		
		// Set the initial timer value to 0
		TCNT1H = 0x00;
		TCNT1L = 0x00;
		
		// Re-enable interrupts for the display
		TIMSK |= (1 << OCIE1A) | (1 << OCIE1B);			
		
		// Enable button timer interrupt
		TIMSK |= (1 << TOIE0);
		
		clock_settings = ReadEEPROM();
		set_display_duty_cycle(clock_settings.brightness);
		
		// Calculate a new software time correction value
		correction_value = 60480000 / (uint32_t)((clock_settings.software_time_correction > 0)?
										clock_settings.software_time_correction:
										clock_settings.software_time_correction * -1);	
	}
}

void init_pins(void) {
	//PC0 = Advance Button	
	//PC6 = 32.768KHz xtal
	//PD2 = PF Sense
	//PD7 = Set Button
	
	DDRA = 0xFF;													//Configure all eight pins of port A as outputs
	DDRB = 0xFF;													//Configure all eight pins of port B as outputs
	DDRC = (1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5)|(1<<PC7);	//Configure output pins, PC0 and PC6 are input
	DDRD = (1<<PD0)|(1<<PD1)|(1<<PD3)|(1<<PD4)|(1<<PD5)|(1<<PD6);	//Configure output pins, PD2 and PD7 are input
	
	PORTA = 0x00;													//Set all eight pins of PORT A low
	PORTB = 0x00;													//Set all eight pins of PORT B low
	PORTC |= (1 << PC0) | (1 << PC6);								//Set output pins low (exclude PC0 and PC6)
	PORTD |= (1 << PD2) | (1 << PD7);								//Set output pins low (exclude PD2 and PD7)
	
	// Setup the interrupts for the power fail sense pin
	//MCUCR |= (1<<ISC01)|(1<<ISC00);		// ISC01=1 ISC00=1 : Rising edge on INT0 triggers interrupt.
	MCUCR |= (1<<ISC00);					// ISC01=0 ISC00=1 : Rising or falling edge on INT0 triggers interrupt.
	GICR  |= (1<<INT0);						// Enable INT0 interrupt
}

int main(void) {
	uint8_t field_values[2] = {0x00, };
	correction_flag = 0;
	correction_counter = 0x0000;			
	correction_value = 0;		
	
	// Clock is initially in the NORMAL state
	clock_state = NORMAL;
	
	// Load settings from EEPROM
	clock_settings = ReadEEPROM();
	
	// Sanity check the settings to see if EEPROM was never initialized
	if (clock_settings.magic_number != MAGIC_NUMBER) {
		WriteDefaultSettings();
		clock_settings = ReadEEPROM();
	}

	// Update the duty cycle
	set_display_duty_cycle(clock_settings.brightness);
	
	// Setup pins
	init_pins();
	
	// Start display update timer
	init_pwm_timer();	
	
	// Exercise the display
	exercise_display(250);

	// Initialize the services	
	init_rtc();				// RTC
	init_event_timer();		// Button timer
	
	while (1) {		
		// Handle flags from the RTC
		switch (sentinal) {
			case QRTR_SECOND:								// Stuff to do at the quarter second mark
				//Unblank the blinky bits in set mode
				if (clock_state == SET)
					display_set_digits(display_new[0], display_new[1], display_new[2], display_colons);
				
				if (set_button_holdoff == 0x01)
					set_button_holdoff++;
				else if (set_button_holdoff >= 0x02)
					set_button_holdoff = 0;
				
				sentinal = CLEAR;
				break;
			case HALF_SECOND:								// Stuff to do at the half second mark
				// Take care of daylight saving time
				if ((clock_settings.daylight_saving_enable) && (!dst_handled)){
					if ((clock.second == 0) &&
					    (clock.month == clock_settings.spring_ahead_month) && 
					    (clock.day == clock_settings.spring_ahead_day) && 
					    (clock.hour == clock_settings.spring_ahead_hour) &&
					    (clock.date <= clock_settings.spring_ahead_week * 7) &&
						(clock.date > (clock_settings.spring_ahead_week - 1) * 7)) {
							clock.hour += 1;
							dst_handled = 1;
					}
					if ((clock.second == 0) &&
					    (clock.month == clock_settings.fall_back_month) && 
					    (clock.day == clock_settings.fall_back_day) && 
					    (clock.hour == clock_settings.fall_back_hour) &&
					    (clock.date <= clock_settings.fall_back_week * 7) &&
						(clock.date > (clock_settings.fall_back_week - 1) * 7)) {
							clock.hour -= 1;
							dst_handled = 1;
					}					
				}				
				
				// In SET mode, blink the bank of digits we're setting
				if (clock_state == SET) {
					switch (set_mode) {
						case YEAR_SET:
						case SEC_SET:
							display_blank_digits(0b00000011);
							break;
						case DATE_SET:
						case MIN_SET:
							display_blank_digits(0b00001100);
							break;
						case MONTH_SET:
						case HOUR_SET:
							display_blank_digits(0b00110000);
							break;
					}
				}	
				
				if (clock_state == NORMAL) {
					// Display or blink the colons depending on mode
					if (clock_settings.blinking_colons == 1) {	// Mode 1: Blink @ .5Hz
						display_colons = 0x03;
					} else if (clock_settings.blinking_colons == 4) {
						display_colons = 0x03;
					}
				}
				
				// Handle the button delay to make them easier to use
				if (set_button_holdoff == 0x01)
					set_button_holdoff++;
				else if (set_button_holdoff >= 0x02)
					set_button_holdoff = 0;
				
				sentinal = CLEAR;
				break;
			case TQRT_SECOND:					//Stuff to do every three quarter of a second mark
				if ((clock_state == SET) || (clock_state == MENU)){
					// Increment the set/menu mode timeout timer
					if (set_timer != 255)
						set_timer++;
				}
				
				// Unblank the blinky bits in set mode
				if (clock_state == SET)
					display_set_digits(display_new[0], display_new[1], display_new[2], display_colons);
				
				// Handle the button delay to make them easier to use
				if (set_button_holdoff == 0x01)
					set_button_holdoff++;
				else if (set_button_holdoff >= 0x02)
					set_button_holdoff = 0;
				
				sentinal = CLEAR;
				break;
			case SECOND:						// Stuff to do at the start of every second
				/*	
				Software Time Correction
				Start a counter (uint32_t) that counts the seconds until we need to add or subtract a second.
				Update this counter and check it in the main loop. Set an int8_t either 1 or -1 each time you're ready to 
				make the change. On the second transition in the RTC counter check for non zero on this and change accordingly
				
				One week is 604,800. Divide 604800 by the correction value to get the counter overflow value
					Ex: If we need to add 1.945 seconds a week, that's one second every 311,752 seconds
				*/
				//correction_flag = 0;	
				if (++correction_counter > correction_value) {
					correction_counter = 0;
					if (clock_settings.software_time_correction < 0)
						correction_flag = -1;
					else	
						correction_flag = 1;
				}			
			
				// Update the global display registers
				if (display_state == NORMAL) {
					// Display or blink the colons depending on mode
					if (clock_settings.blinking_colons == 1) {			// Mode 1: .5hz
						display_colons = 0x00;
					} else if (clock_settings.blinking_colons == 2) { 	// Mode 2: 1 hz
						if (display_colons == 0x00) {
							display_colons = 0x03;
						} else {
							display_colons = 0x00;
						}
					} else if (clock_settings.blinking_colons == 3) {	// Mode 3: AM/PM
						if (clock.hour > 11) {
							display_colons == 0x03;
						} else {
							display_colons == 0x00;
						}
					} else if (clock_settings.blinking_colons == 4) {	// Mode 4: On
						display_colons = 0x03;
					}
					
					// Display the time or date depending on mode
					display_new[1] = clock.minute;
					display_new[2] = clock.second;
					// Handle 12/24 hour mode
					if (!clock_settings.clock_display_24hr) {
						display_new[0] = clock.hour % 12;
						if (display_new[0] == 0)
							display_new[0] = 12;
					} else {
						display_new[0] = clock.hour;
					}
					
				// Display is in date mode, display that instead
				} else if (display_state == DATE) {
					display_new[0] = clock.month;
					display_new[1] = clock.date;
					display_new[2] = clock.year;
				}

				// If we're in SET mode, blink the bank of digits we're setting
				if (clock_state == SET) {
					switch (set_mode) {				
						case YEAR_SET:
						case SEC_SET:
							display_blank_digits(0b00000011);
							break;
						case DATE_SET:
						case MIN_SET:
							display_blank_digits(0b00001100);
							break;
						case MONTH_SET:
						case HOUR_SET:
							display_blank_digits(0b00110000);
							break;
					}
				}	
				
				// Periodically display the date if enabled
				if ((clock_settings.display_date) &&
					(clock_state == NORMAL) &&
					(clock.second >= clock_settings.display_date_at_seconds) &&
					(clock.second < clock_settings.display_date_at_seconds + clock_settings.display_date_duration)) {
					display_new[0] = clock.month;
					display_new[1] = clock.date;
					display_new[2] = clock.year;		
					if (clock_settings.blinking_colons_during_date == 1)
						display_colons = 0x03;
					else if (clock_settings.blinking_colons_during_date == 0)
						display_colons = 0x00;
				}
				
				// start a fade and set compa to the extreme
				OCR1AL = display_duty_cycle;
				
				// Do some stuff to slow down button response and make it more usable
				if (set_button_holdoff == 0x01)
					set_button_holdoff++;
				else if (set_button_holdoff >= 0x02)
					set_button_holdoff = 0;
				
				sentinal = CLEAR;
				break;
		}
		// do the business, handle various modes of the clock
		switch (clock_state) {
			case NORMAL:
				override_pwm = FALSE;							// Don't force disable PWM
				clock_settings.crossfade_enable = TRUE;			// Enable crossfade
				
				// Handle Cathode Poisoning Prevention routine here
				if (clock_settings.cathode_poison_prevention_enabled) {
					if ((clock.hour >= clock_settings.cathode_poison_start_hour) && 
					(clock.hour < clock_settings.cathode_poison_start_hour + clock_settings.cathode_poisoning_duration)) {
						cathode_poison_routine();
					}
				}
				
				// Has the user done anything with the buttons?
				if ((set_button_flag == LONG_PRESS) && (adv_button_flag == LONG_PRESS)) {
				// Entered setup menu
					override_pwm = TRUE;						// Force full brightness
					clock_settings.crossfade_enable = FALSE;	// Turn off the fade
					clock_state = MENU;							// State machine update, we're in the menu
					display_state = MENU;						// We're in the menu, let the display stuff know
					menu_option = 1;							// Start back at option one in the menu
					set_timer = 0;								// Start the timeout timer
					TIMSK &= ~((1 << OCIE1A) | (1 << OCIE1B));	// disable interrupts for the display
					set_button_holdoff = 0x01;
				} else if ((set_button_flag == LONG_PRESS) && (ADV_BUTTON_PORT & (1 << ADV_BUTTON_IDX))){
				// Entered Set mode
					override_pwm = TRUE;						// Force full brightness
					clock_settings.crossfade_enable = FALSE;	// Disable crossfade
					clock_state = SET;							// State machine update, we're in set mode
					display_state = NORMAL;						// Tell the display to show the time
					set_mode = SEC_SET;							// Set the initial field to update
					set_timer = 0;								// Start the timeout timer
					TIMSK &= ~((1 << OCIE1A) | (1 << OCIE1B));	// disable interrupts for the display
					set_button_holdoff = 0x01;
					display_set_digits(display_new[0], display_new[1], display_new[2], display_colons);
				} else if ((adv_button_flag == LONG_PRESS) && (SET_BUTTON_PORT & (1 << SET_BUTTON_IDX))) {
					// Set to 'Date Only' mode
					display_state = DATE;						// Tell the display to show the date
					clock_state = DATE;
				}
				break;
			case SET:	
				// Has the user done anything with the buttons?
				if ((set_button_flag == SHORT_PRESS) && (!set_button_holdoff)) {
				// Cycle through the settable fields and blink whatver one we're in
					set_button_holdoff = 0x01;					// set the holdoff
					set_timer = 0;								// Clear the menu timeout timer
					switch (set_mode) {							// Cycle through the fields
						case SEC_SET:
							set_mode = MIN_SET;	
							display_state = NORMAL;
							display_blank_digits(0b00000011);
							break;
						case MIN_SET:
							set_mode = HOUR_SET;
							display_state = NORMAL;
							display_blank_digits(0b00001100);
							break;
						case HOUR_SET:
							set_mode = MONTH_SET;
							display_state = DATE;
							display_blank_digits(0b00110000);
							break;
						case MONTH_SET:
							set_mode = DATE_SET;
							display_state = DATE;
							display_blank_digits(0b00110000);
							break;
						case DATE_SET:
							set_mode = YEAR_SET;
							display_state = DATE;
							display_blank_digits(0b00001100);
							break;
						case YEAR_SET:
							set_mode = SEC_SET;
							display_state = NORMAL;
							display_blank_digits(0b00000011);
							break;
					}
					update_display();				// Update the display since we just entered Set mode
				}else if (adv_button_flag == SHORT_PRESS) {	
				// Increment the field
					set_timer = 0;
					//adv_button_counter = 0;
					adv_button_flag = NOT_PRESSED;
					increment_time_date(set_mode);	// increment whatever field we're in
					update_display();	// Update the display since we just changed a value and dont want to wait for the next interrupt
				}
				else if ((adv_button_flag == CONTINUED_PRESS)){// && (sentinal = HALF_SECOND)) {	
				// Handle repeated increments
					sentinal = CLEAR;
					set_timer = 0;
					adv_button_counter = CONT_PRESS_TIMEOUT - 5;
					adv_button_flag = NOT_PRESSED;					
					increment_time_date(set_mode);	// increment whatever field we're in
					update_display();				// Update the display since we just changed a value and dont want to wait for the next interrupt
				}
				
				if ((clock.hour > 11) && (!clock_settings.clock_display_24hr) && (display_state == NORMAL))
					display_colons = 2;				// Set a little PM indicator
				else if (display_state == DATE)
					display_colons = 3;				// Set both colons on for date set
				else
					display_colons = 0;				// Clear the colons for time set
				
				// Have we timed out in this mode?
				if (set_timer > 10) {
					display_set_digits(display_old[0], display_old[1], display_old[2], display_colons);
					TCNT1H = 0x00;									// Set the initial timer value to 0
					TCNT1L = 0x00;
					TIMSK |= (1 << OCIE1A) | (1 << OCIE1B);			// Re-enable interrupts for the display
					clock_state = NORMAL;
					display_state = NORMAL;
					set_timer = 255;
				}
				break;
			case MENU:
				TIMSK &= ~((1 << OCIE1A) | (1 << OCIE1B));		// disable interrupts for the display, force disable PWM
				
				// Draw the menu on the display
				display_colons = 0x00;
				read_menu_setting(field_values, menu_option);	// Setup the field_values array with the values for this menu option
				display_new[0] = menu_option;
				display_new[1] = field_values[0];
				display_new[2] = field_values[1];
				display_set_digits(display_new[0], display_new[1], display_new[2], display_colons);

				// Blank out the unused digits
				if (field_values[0] == 0)
					display_blank_digits(0b00001100);
				else if ((field_values[0] / 10) == 0)
					display_blank_digits(0b00001000);
			
				// Handle set button press, cycle through the menu options
				if ((set_button_flag == SHORT_PRESS) && (!set_button_holdoff)) {
					// set the holdoff
					set_timer = 0;							// Clear the menu timeout timer
					set_button_holdoff = 0x01;
					swap = 1;								// clear swap, we aren't holding cont press
					// Cycle through the options to set
					if (++menu_option > MENU_OPTION_COUNT)
						menu_option = 1;
				}else if (adv_button_flag == SHORT_PRESS) {	
				// Increment the field
					set_timer = 0;
					adv_button_counter = 0;
					adv_button_flag = NOT_PRESSED;
					increment_menu_setting(menu_option, 0);
				} 
				else if ((adv_button_flag == CONTINUED_PRESS) && (sentinal = HALF_SECOND)) {
				// Handle repeated increments
					sentinal = CLEAR;
					set_timer = 0;
					adv_button_counter = CONT_PRESS_TIMEOUT - 5;
					adv_button_flag = NOT_PRESSED;
					increment_menu_setting(menu_option, 1);
				}
				
				// Have we timed out in this mode?
				if (set_timer > 10) {
					// Commit the changes to EEPROM
					UpdateSettings(clock_settings);
					//	Set brightness
					set_display_duty_cycle(clock_settings.brightness);
					// Calculate a new software time correction value
					correction_value = 60480000 / (uint32_t)((clock_settings.software_time_correction > 0)?
													clock_settings.software_time_correction:
													clock_settings.software_time_correction * -1);
					// Update the display digits
					display_set_digits(display_old[0], display_old[1], display_old[2], display_colons);
					
					// Set the initial timer value to 0
					TCNT1H = 0x00;
					TCNT1L = 0x00;
					// Re-enable interrupts for the display
					TIMSK |= (1 << OCIE1A) | (1 << OCIE1B);
					// Exit menu mode
					clock_state = NORMAL;
					display_state = NORMAL;
					// Disable the time-out timer
					set_timer = 255;
				}
				break;
			case DATE:
			/*
				// Hijacked for testing purposes!
				display_colons = 0x00;
				display_new[0] = correction_counter / 10000;
				display_new[1] = correction_counter % 10000 / 100;
				display_new[2] = correction_counter % 100;
				display_set_digits(display_new[0], display_new[1], display_new[2], display_colons);
				if ((set_button_flag == SHORT_PRESS) || (adv_button_flag == SHORT_PRESS)) {
					clock_state = NORMAL;
					display_state = NORMAL;
				}
			*/
				// Date only display mode, more of a novelty but requested by a user so here it is
				// Lock the clock to date display only
				display_new[0] = clock.month;
				display_new[1] = clock.date;
				display_new[2] = clock.year;
				display_set_digits(display_new[0], display_new[1], display_new[2], display_colons);
				if ((set_button_flag == SHORT_PRESS) || (adv_button_flag == SHORT_PRESS)) {
					clock_state = NORMAL;
					display_state = NORMAL;
				}
				break;
		}
	}
}

void update_display(void) {
	// refresh the display, used in menus and set mode etc
	if (display_state == NORMAL) {
		display_new[0] = clock.hour;
		display_new[1] = clock.minute;
		display_new[2] = clock.second;
		if (!clock_settings.clock_display_24hr) {	// Handle 12/24 hour mode
			display_new[0] %= 12;
			if (display_new[0] == 0)
				display_new[0] = 12;	
		}
	} else if (display_state == DATE) {
		display_new[0] = clock.month;
		display_new[1] = clock.date;
		display_new[2] = clock.year;
	}
	display_set_digits(display_new[0], display_new[1], display_new[2], display_colons);
}

void increment_time_date(uint8_t set_mode) {
	// set mode increments
	switch (set_mode) {
		case SEC_SET:
			clock.second = 0;
			break;
		case MIN_SET:
			if (++clock.minute == 60)
				clock.minute = 0;
			break;
		case HOUR_SET:
			if (++clock.hour == 24)
				clock.hour = 0;
			break;
		case MONTH_SET:
			if (++clock.month == 13)
				clock.month = 1;
			break;
		case DATE_SET:
			if (++clock.date==32) {
				clock.date=1;
			} else if (clock.date==31) {
				if ((clock.month==4) || (clock.month==6) || (clock.month==9) || (clock.month==11)) {
					clock.date=1;
				}
			} else if (clock.date==30) {
				if(clock.month==2) {
					clock.date=1;
				}
			} else if (clock.date==29) {
				if((clock.month==2) && (not_leap())) {
					clock.date=1;
				}
			}

			clock.day = calculate_day_of_week();	
			break;
		case YEAR_SET:
			if (++clock.year==100)
				clock.year = 0;
			break;
	}
	correction_counter = 0;			// We changed the time, restart counter for software time correction
}

void read_menu_setting(uint8_t field_values[2], uint8_t menu_option) {
	int16_t temp_val = 0;
	display_colons = 0x00;
	clock.day = calculate_day_of_week();
	field_values[0] = 0;
	field_values[1] = 0;
	switch (menu_option) {
		case 1:
			field_values[1] = (clock_settings.clock_display_24hr)?24:12;
			break;
		case 2:
			field_values[1] = clock_settings.leading_zero_blank;
			break;
		case 3:
			field_values[1] = clock_settings.crossfade_enable;
			break;
		case 4:
			field_values[1] = clock_settings.crossfade_step;
			break;
		case 5:
			field_values[1] = clock_settings.blinking_colons;
			break;
		case 6:
			field_values[1] = clock_settings.blinking_colons_during_date;
			break;
		case 7:
			field_values[1] = clock_settings.display_date;
			break;
		case 8:
			field_values[1] = clock_settings.display_date_at_seconds;
			break;
		case 9:
			field_values[1] = clock_settings.display_date_duration;
			break;
		case 10:
			field_values[1] = clock_settings.brightness % 100;
			field_values[0] = clock_settings.brightness / 100;
			break;
		case 11:
			field_values[1] = clock_settings.cathode_poison_prevention_enabled;
			break;
		case 12:
			field_values[1] = clock_settings.cathode_poison_start_hour;
			break;
		case 13:
			field_values[1] = clock_settings.cathode_poisoning_duration;
			break;
		case 14:
			field_values[1] = clock_settings.daylight_saving_enable;
			break;
		case 15:
			field_values[1] = clock_settings.spring_ahead_hour;
			break;
		case 16:
			field_values[1] = clock_settings.spring_ahead_day;
			break;
		case 17:
			field_values[1] = clock_settings.spring_ahead_week;
			break;
		case 18:
			field_values[1] = clock_settings.spring_ahead_month;
			break;
		case 19:
			field_values[1] = clock_settings.fall_back_hour;
			break;
		case 20:
			field_values[1] = clock_settings.fall_back_day;
			break;
		case 21:
			field_values[1] = clock_settings.fall_back_week;
			break;
		case 22:
			field_values[1] = clock_settings.fall_back_month;
			break;
		case 23:
			field_values[1] = clock_settings.pwm_freq % 100;
			field_values[0] = clock_settings.pwm_freq / 100;
			break;
		case 24:
			temp_val = (clock_settings.software_time_correction > 0)?clock_settings.software_time_correction:(-1 * clock_settings.software_time_correction);
			display_colons = 0x01;
			if (clock_settings.software_time_correction < 0) {
				display_colons = 0x03;		// Negative number
				field_values[1] = (uint8_t)(temp_val % 100);
				field_values[0] = (uint8_t)(temp_val / 100);			
			} else {
				display_colons = 0x01;		// Positive number
				field_values[1] = (uint8_t)(temp_val % 100);
				field_values[0] = (uint8_t)(temp_val / 100);			
			}
			break;
		case 25:
			field_values[1] = clock.day;
			break;
	}
}

void increment_menu_setting(uint8_t menu_option, uint8_t speed) {
	switch (menu_option) {
		case 1:
			if (++clock_settings.clock_display_24hr > 1)
				clock_settings.clock_display_24hr = 0;
			break;
		case 2:
			if (++clock_settings.leading_zero_blank > 1)
				clock_settings.leading_zero_blank = 0;
			break;
		case 3:
			if (++clock_settings.crossfade_enable > 1)
				clock_settings.crossfade_enable = 0;
			break;
		case 4:
			if (++clock_settings.crossfade_step > 10)
				clock_settings.crossfade_step = 1;
			break;
		case 5:
			if (++clock_settings.blinking_colons > 4)
				clock_settings.blinking_colons = 0;
			break;
		case 6:
			if (++clock_settings.blinking_colons_during_date > 2)
				clock_settings.blinking_colons_during_date = 0;
			break;
		case 7:
			if (++clock_settings.display_date > 1)
				clock_settings.blinking_colons_during_date = 0;
			break;
		case 8:
			if (clock_settings.display_date_at_seconds < 50)
				clock_settings.display_date_at_seconds += 10;
			else
				clock_settings.display_date_at_seconds = 0;
			break;
		case 9:
			if (++clock_settings.display_date_duration > 10)
				clock_settings.display_date_duration = 1;
			break;
		case 10:
			if (clock_settings.brightness < 100)
				clock_settings.brightness += 10;
			else
				clock_settings.brightness = 10;
			break;
		case 11:
			if (++clock_settings.cathode_poison_prevention_enabled > 1)
				clock_settings.cathode_poison_prevention_enabled = 0;
			break;
		case 12:
			if (++clock_settings.cathode_poison_start_hour > 23)
				clock_settings.cathode_poison_start_hour = 0;
			break;
		case 13:
			if (++clock_settings.cathode_poisoning_duration > 12)
				clock_settings.cathode_poisoning_duration = 1;
			break;
		case 14:
			if (++clock_settings.daylight_saving_enable > 1)
				clock_settings.daylight_saving_enable = 0;
			break;
		case 15:
			if (++clock_settings.spring_ahead_hour > 22)
				clock_settings.spring_ahead_hour = 1;
			break;
		case 16:
			if (++clock_settings.spring_ahead_day > 6)
				clock_settings.spring_ahead_day = 0;
			break;
		case 17:
			if (++clock_settings.spring_ahead_week > 4)
				clock_settings.spring_ahead_week = 1;
			break;
		case 18:
			if (++clock_settings.spring_ahead_month > 12)
				clock_settings.spring_ahead_month = 1;
			break;
		case 19:
			if (++clock_settings.fall_back_hour > 22)
				clock_settings.fall_back_hour = 1;
			break;
		case 20:
			if (++clock_settings.fall_back_day > 6)
				clock_settings.fall_back_day = 0;
			break;
		case 21:
			if (++clock_settings.fall_back_week > 4)
				clock_settings.fall_back_week = 1;
			break;
		case 22:
			if (++clock_settings.fall_back_month > 12)
				clock_settings.fall_back_month = 1;
			break;
		case 23:
		//	field_value[1] = clock_settings.pwm_freq;
			break;
		case 24:
			if (speed) {
			// count in tens
				if (inc_dec) {
					// count up
					if (clock_settings.software_time_correction > 315) {
						clock_settings.software_time_correction = -325;
					} else {
						clock_settings.software_time_correction += 10;
					}
				} else {
					// count down
					if (clock_settings.software_time_correction < -315) {
						clock_settings.software_time_correction = 325;
					} else {
						clock_settings.software_time_correction -= 10;
					}
				}
			} else {
			// count in ones
				if (inc_dec) {
					// count up
					if (++clock_settings.software_time_correction > 325) {
						clock_settings.software_time_correction = -325;
					}
				} else {
					// count down
					if (--clock_settings.software_time_correction < -325) {
						clock_settings.software_time_correction = 325;
					}
				}
			}
			break;
	}
}

void exercise_display(uint16_t delay_ms) {
// Demand full brightness and no crossface for the glorious exercise
	TIMSK &= ~((1 << OCIE1A) | (1 << OCIE1B));	// disable interrupts for the display
	for (int i = 0; i < 10; i++) {
		int temp = i * 10 + i;
		display_new[0] = temp;
		display_new[1] = temp;
		display_new[2] = temp;
		display_set_digits(display_new[0], display_new[1], display_new[2], 0x03);
		_delay_ms(delay_ms);
	}
	display_new[0] = clock.hour;
	display_new[1] = clock.minute;
	display_new[2] = clock.second;
	TIMSK |= (1 << OCIE1A) | (1 << OCIE1B);		// Enable interrupts on timer 1 A and B compare
	TCNT1H = 0x00;								// Set the initial timer value to 0
	TCNT1L = 0x00;
	OCR1AL = display_duty_cycle;				// start a fade and set compa to the extreme
}

void cathode_poison_routine(void) {
	uint8_t temp;	
	// Demand full brightness and no crossface
	TIMSK &= ~((1 << OCIE1A) | (1 << OCIE1B));	// disable interrupts for the display
	display_state == CP_SERVICE;
	while (clock.hour < (clock_settings.cathode_poison_start_hour + clock_settings.cathode_poisoning_duration)) {
		if (clock.second < 3) {
			display_new[0] = clock.hour;
			display_new[1] = clock.minute;
			display_new[2] = clock.second;			
			if (!clock_settings.clock_display_24hr) {
				display_new[0] %= 12;
				if (display_new[0] == 0)
					display_new[0] = 12;	
			}
			display_set_digits(display_new[0], display_new[1], display_new[2], 0x00);
		} else {
			uint8_t temp = clock.minute / 6;
			temp = temp * 10 + temp;
			display_set_digits(temp, temp, temp, 0x03);
		}
		// Check if the user is doing something to the buttons
		if ((set_button_flag != NOT_PRESSED) || (adv_button_flag != NOT_PRESSED))
			break;
		// Check for a power failure
		if (!(PF_PORT & (1 << PF_PIN)))
			break;
	}
	TIMSK |= (1 << OCIE1A) | (1 << OCIE1B);		// Enable interrupts on timer 1 A and B compare
	TCNT1H = 0x00;								// Set the initial timer value to 0
	TCNT1L = 0x00;
	OCR1AL = display_duty_cycle;				// start a fade and set compa to the extreme
	display_state = NORMAL;
}

clock_settings_t ReadEEPROM(void){
	clock_settings_t temp;
	eeprom_read_block((void*)&temp, (const void*)&clock_settings_eeprom, sizeof(clock_settings_t));
  
	correction_value = 60480000 / (uint32_t)((clock_settings.software_time_correction > 0)?
									clock_settings.software_time_correction:
									clock_settings.software_time_correction * -1);
   return(temp);
}

void UpdateSettings(clock_settings_t a){
	eeprom_update_block((const void*)&a, (void*)&clock_settings_eeprom, sizeof(clock_settings_t));
	//eeprom_write_block((const void*)&a, (void*)&clock_settings_eeprom, sizeof(clock_settings_t));
}

void WriteDefaultSettings(void) {
	// Initialize the default values
	clock_settings.clock_display_24hr = FALSE;					//Opt 1:	 1 = 24hr mode										def: 0
	clock_settings.leading_zero_blank = FALSE;					//Opt 2:	 1 = enabled, blank 0								def: 0
	clock_settings.crossfade_enable = TRUE;						//Opt 3:	 1 = enable crossfade								def: 1
	clock_settings.crossfade_step = 3;							//Opt 4:	 1 to 10, 1 is quick 10 is slow						def: 3
	clock_settings.blinking_colons = 2;							//Opt 5:	 0:off 1:blink @ .5hz 2:blink @ 1hz 3:AM/PM 4: On	def: 1
	clock_settings.blinking_colons_during_date = 1;				//Opt 6:	 0 ignore, 1 on, 2 off								def: 1
	clock_settings.display_date = TRUE;							//Opt 7:	 1 display date periodically						def: 1
	clock_settings.display_date_at_seconds = 40;				//Opt 8:	 (0, 10, 20, 30, 40, 50)							def: 40
	clock_settings.display_date_duration = 1;					//Opt 9:	 seconds to display date for ex: 2					def: 2
	clock_settings.brightness = 100;							//Opt 10:	 steps of 10 10-100									def: 100
	clock_settings.cathode_poison_prevention_enabled = TRUE;	//Opt 11:	 0 disable 1 enable									def: 1
	clock_settings.cathode_poison_start_hour = 3;				//Opt 12:	 0-23												def: 3
	clock_settings.cathode_poisoning_duration = 1;				//Opt 13:	 how many hours to stay in mode						def: 1
	clock_settings.daylight_saving_enable = TRUE;				//Opt 14:	 1 = enable											def: 1
																			 // Default is second Sunday of March at 2:00AM	
	clock_settings.spring_ahead_hour = 2;						//Opt 15: 														def: 2
	clock_settings.spring_ahead_day = 0;						//Opt 16:														def: 0
	clock_settings.spring_ahead_week = 2;						//Opt 17:														def: 2
	clock_settings.spring_ahead_month = 3;						//Opt 18:														def: 3
																			 // Default is first Sunday of November at 2:00AM
	clock_settings.fall_back_hour = 2;							//Opt 19:														def: 2
	clock_settings.fall_back_day = 0;							//Opt 20:														def: 0
	clock_settings.fall_back_week = 1;							//Opt 21:														def: 1
	clock_settings.fall_back_month = 11;						//Opt 22:														def: 11
	clock_settings.pwm_freq = 0xFF;								//Opt 23:														R/O
	clock_settings.software_time_correction = 194;				//Opt 24:	number of seconds per week to add or subtract		def: 1.94
																			// 	left colon on for negative numbers
																//Opt 25: Day of week 0:Sunday 6:Saturday						R/O
	clock_settings.magic_number = MAGIC_NUMBER;					// Used to see if the EEPROM has been set
	UpdateSettings(clock_settings);
}

void set_display_duty_cycle(uint8_t brightness) {
	// Convert brightness percentage to a proper duty cycle value
	switch (brightness) {
		case 10:
			display_duty_cycle = 10;
			break;
		case 20:
			display_duty_cycle = 35;
			break;
		case 30:
			display_duty_cycle = 60;
			break;
		case 40:
			display_duty_cycle = 85;
			break;
		case 50:
			display_duty_cycle = 110;
			break;
		case 60:
			display_duty_cycle = 135;
			break;
		case 70:
			display_duty_cycle = 160;
			break;
		case 80:
			display_duty_cycle = 185;
			break;
		case 90:
			display_duty_cycle = 210;
			break;
		case 100:
			display_duty_cycle = 240;
			break;
		default:
			display_duty_cycle = 240;
	}
}

uint8_t calculate_day_of_week(void) {
	// Takes the date and returns the day of week
	// Sunday = 0, Monday = 1 ... Saturday = 6
	// **Only works for years in the 2000's
	uint8_t temp;
	temp = clock.year / 4;
	temp += clock.date;
	switch (clock.month) {
		case 1:
		case 10:
			temp += 1;
			break;
		case 5:
			temp += 2;
			break;
		case 2:
		case 3:
		case 11:
			temp += 4;
			break;
		case 4:
		case 7:
			break;
		case 9:
		case 12:
			temp += 6;
			break;
		case 6:
			temp += 5;
			break;
		case 8:
			temp += 3;
			break;
	}
	
	if (!(clock.year % 4))
		if ((clock.month == 1) || (clock.month == 2))
			temp -= 1;
	
	temp += 6;
	temp += clock.year;
	temp %= 7;
	
	if (temp == 0)
		temp = 7;
	
	return temp - 1;
}