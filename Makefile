# vim: set tabstop=4 shiftwidth=4 expandtab :
#
# nixietherm-firmware - NixieClock Mega Main Firmware Program
# Copyright (C) 2020 Edward Koloski
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http:#www.gnu.org/licenses/>.

PORT			= usb
MCU				= atmega16
#Internal RC clock
F_CPU 			= 8000000
AVRDUDE			= avrdude
FORMAT			= ihex
TARGET			= nixieclock-firmware
PROGRAMMER		= dragon_isp

CROSS_COMPILE	= avr-

CC				= $(CROSS_COMPILE)gcc
LD				= $(CROSS_COMPILE)ld
OBJCOPY			= $(CROSS_COMPILE)objcopy
OBJDUMP			= $(CROSS_COMPILE)objdump
SIZE			= $(CROSS_COMPILE)size

RM				= rm
ECHO			= echo

CFLAGS			+= -g
#CFLAGS			+= -ggdb3
#CFLAGS			+= -gstabs
CFLAGS			+= -Os
CFLAGE			+= -Wall
CFLAGS			+= -Werror
CFLAGS 			+= -Wstrict-prototypes
CFLAGS			+= -mcall-prologues
CFLAGS			+= -std=c99
CFLAGS			+= -mmcu=$(MCU)
CFLAGS 			+= -DF_CPU=$(F_CPU)
#CFLAGS			+= -save-temps

LDFLAGS			= -Wl,-gc-sections 
LDFLAGS			+= -Wl,-relax
LDFLAGS			+= -Wl,-Map,$(TARGET).map

BINFILE			= nixieclock-firmware.bin
ELFFILE			= nixieclock-firmware.elf
ASMFILE			= nixieclock-firmware.s
EEPFILE			= nixieclock-eeprom.eep

OBJS			+= system/nixie.o
OBJS			+= system/display.o
OBJS			+= system/rtc.o
OBJS			+= system/buttons.o

EEPROMOPTS		+= -O $(FORMAT)
EEPROMOPTS		+= -j .eeprom
EEPROMOPTS		+= --preserve-dates
EEPROMOPTS		+= --no-change-warnings
EEPROMOPTS		+= --change-section-lma=.eeprom=0
EEPROMOPTS		+= --set-section-flags=.eeprom=alloc,load

all: $(BINFILE)
-include $(OBJS:%.o=%.d)

.PHONY: clean
clean:
	$(RM) -f $(BINFILE) $(ELFFILE) $(OBJS) $(OBJS:%.o=%.d) $(TARGET).map *.s *.i *.hex

$(ELFFILE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

.PHONY: eep
%.eep:$(ELFNAME)
	@echo
	@echo $(MSG_EEPROM) $@
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O $(FORMAT) $< $(EEPFILE)

$(BINFILE): $(ELFFILE)
	$(OBJCOPY) -j .text -j .data -O $(FORMAT) $(ELFFILE) $(BINFILE)
	$(SIZE) -C --mcu=$(MCU) $(ELFFILE)

disasm: $(ELFNAME)
	$(OBJDUMP) -drS --show-raw-insn $(ELFFILE)

%.hex: %.obj
	rm -f $(TARGET).hex
	$(OBJCOPY) -j .text -j .data -O ihex $(ELFFILE) $(TARGET).hex
	$(SIZE) $(ELFFILE)

%.obj: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

#asm:%.hex
#	$(CC) $(CFLAGS) -S $(OBJS) #C FILES!
#	rm -f %.hex

.PHONY: program
program:$(BINFILE)
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -U flash:w:$(BINFILE)
	
.PHONY: program-eeprom
program-eeprom:$(BINFILE)
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -U eeprom:w:$(ELFFILE)