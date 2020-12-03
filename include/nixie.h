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

#ifndef __NIXIE_H_
#define __NIXIE_H_

// Constants for the power fail sense pin PD2
#define PF_PORT		PORTD
#define PF_PIN		2

// Constants for the clock and display state machines
#define NORMAL		0
#define SET			1
#define MENU		2
#define DATE		3
#define CP_SERVICE	4

// Menu option count, how many do we have now?
#define MENU_OPTION_COUNT	25

#define TRUE		1
#define FALSE		0

#include <avr/eeprom.h>

// 26 of 64 bytes in EEPROM used
typedef struct _clock_settings_t {
	uint8_t 	clock_display_24hr;
	uint8_t 	leading_zero_blank;
	uint8_t 	crossfade_enable;
	uint8_t		crossfade_step;
	uint8_t		blinking_colons;
	uint8_t		blinking_colons_during_date;
	uint8_t		display_date;
	uint8_t 	display_date_at_seconds;
	uint8_t		display_date_duration;
	uint8_t		brightness;
	uint8_t		cathode_poison_prevention_enabled;
	uint8_t		cathode_poison_start_hour;
	uint8_t		cathode_poisoning_duration;
	uint8_t		daylight_saving_enable;
	uint8_t		spring_ahead_hour;
	uint8_t		spring_ahead_day;
	uint8_t		spring_ahead_week;
	uint8_t		spring_ahead_month;
	uint8_t		fall_back_hour;
	uint8_t		fall_back_day;
	uint8_t		fall_back_week;
	uint8_t		fall_back_month;
	uint8_t		pwm_freq;
	int16_t		software_time_correction;
	uint8_t		magic_number;
} clock_settings_t;

extern volatile clock_settings_t clock_settings;
extern volatile EEMEM clock_settings_t clock_settings_eeprom;
extern volatile uint8_t set_mode;
extern volatile uint8_t clock_state;
extern volatile int8_t correction_flag;
extern volatile uint32_t correction_counter;			
extern volatile uint32_t correction_value;
extern volatile uint8_t inc_dec, swap;
volatile uint8_t menu_option;

int main(void);
void exercise_display(uint16_t delay_ms);
void cathode_poison_routine(void);
void init_pins(void);
void WriteDefaultSettings(void);
void UpdateSettings(clock_settings_t a);
void read_menu_setting(uint8_t field_values[2], uint8_t menu_option);
void increment_menu_setting(uint8_t menu_option, uint8_t speed);
void increment_time_date(uint8_t set_mode);
void update_display(void);
void set_display_duty_cycle(uint8_t brightness);
uint8_t calculate_day_of_week(void);
clock_settings_t ReadEEPROM(void);

#endif // __NIXIE_H_