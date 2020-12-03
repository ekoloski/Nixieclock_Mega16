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

#ifndef __BUTTONS_H_
#define __BUTTONS_H_

#define SET_BUTTON_PORT		PIND
#define SET_BUTTON_IDX		7
#define ADV_BUTTON_PORT		PINC
#define ADV_BUTTON_IDX		0

#define NOT_PRESSED		0
#define SHORT_PRESS		1
#define LONG_PRESS		2
#define CONTINUED_PRESS 3

#define SHORT_PRESS_TIMEOUT		4
#define LONG_PRESS_TIMEOUT		56  //96
#define CONT_PRESS_TIMEOUT		64	//128

extern volatile uint8_t set_button_counter;
extern volatile uint8_t adv_button_counter;
extern volatile uint8_t set_button_flag, adv_button_flag;
extern volatile uint8_t set_button_holdoff;

void init_event_timer(void);

#endif // __BUTTONS_H_