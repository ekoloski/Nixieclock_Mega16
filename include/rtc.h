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

#ifndef __RTC_H_
#define __RTC_H_

#define CLEAR		0
#define QRTR_SECOND	1
#define HALF_SECOND	2
#define TQRT_SECOND	3
#define SECOND		4

typedef struct _timespec_t {
    uint16_t year;
    uint8_t  month;
    uint8_t  date;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} timespec_t;

extern volatile timespec_t clock;
extern volatile uint8_t set_timer;
extern volatile uint8_t sentinal;
extern volatile uint8_t unlock_correction;
extern volatile int8_t correction;

extern char not_leap(void);
extern void init_rtc(void);

#endif // __RTC_H_