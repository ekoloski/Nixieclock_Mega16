# Nixieclock_Mega16
6 digit BCD Nixie Tube clock based on Atmel ATMega16 AVR
## Basic operation
### Setting the clock:
	The clock will default to 12:00:00 Midnight, (or 00:00:00 depending on 12/24 hour mode). 
	To enter ‘Set’ mode hold the Set button for a moment. The seconds digits will begin to 
	flash to indicate that the clock is now in ‘Set’ mode. Press the ‘Set’ button to choose 
	between the various fields incrementing up trhrough the date. they will loop back to 
	seconds after each one has been visited. 
	Press the ‘Adv’ button momentarily to advance the field by one, or hold the button for 
	larger changes. The clock will return to normal mode after a few moments with no buttons 
	pressed. Timekeeping is maintained through brief power failures by means of an integrated 
	supercapacitor. This does not last as long as a backup battery, however it (in theory) 
	should never need to be replaced.

### Menu settings:
	The clock has many menu options. To enter the menu mode press and hold both the Set 
	and Adv buttons at the same time. After a few seconds the display will change to 
	indicate that it is now in ‘Menu’ mode. The hours tubes will indicate which menu option 
	is selected, the the Minutes and Seconds tubes will indicate setting. Pressing the Set 
	button advances to the next menu option, pressing the Adv button changes the value. 
	The clock will return to normal mode after a few moments with no buttons pressed. 
	Settings are stored in nonvolatile EEPROM and are retained through power outages or reset.

## Menu options:
* **Opt 1: 	12/24 Hour Mode**	 (Default: 12)
	Display time in 12 hour or 24 hour (Military) mode
		12
		24
* **Opt 2: 	Leading Zero Blanking**	 (Default: 0)
	Blanks the leading zero
		1:*enable*
		0:*disable*
* **Opt 3:  Digit Crossfade**		(Default: 1)
	When changing digits on the display, fade in the new one
		1:*enabled*
		0:*disable*
* **Opt 4:  Crossfade Step**		(Default: 3)
	How quickly the crossfade happens
		1-10 *quick to slow*
* **Opt 5:  Blinking Colons** 		(Default: 2)
		0:*Off
		1:*Blink @ .5hz*
		2:*Blink @ 1hz*
		3:*AM/PM (PM On)*
		4:*Constant On*
* **Opt 6:  Colon Behavior During Date Display**	(Default: 1)
	What should the colons do during date display mode
		0:*Follow behavior defined in Option 5*
		1:*On*
		2:*Off*
* **Opt 7:  Display Date Periodically**			(Default: 1)
	Display the date every minute for a few seconds
		1:*Enable*
		0:*Disable*
* **Opt 8:  When to display the date**		(Default: 40)
		0
		10
		20
		30
		40
		50
* **Opt 9:   Display Date Duration**			(Default: 2)
		1-10	*Seconds*
* **Opt 10: Display Brightness**			(Default: 100)
		10-100  *(in steps of 10)*
* **Opt 11: Cathode Poisoning Prevention**		(Default: 1)
	Nixie tubes can sometimes fail due to burn-in, if one cathode 
	is left energized for too long it can sputter 	material onto 
	adjacent cathodes and shorten their lifetime. The cathode 
	poisoning prevention service exercises each cathode for a 
	minimum of 6 minutes at full brightness in an attempt to 
	burn off any accumulated debris. This service can be enabled 
	and will run for a minimum of 1 hour or a maximum of 12 hours. 
	The time will be displayed for a couple of seconds at each 
	minute transition if the service is running.
		1:*enable*
		0:*disable*
* **Opt 12: Cathode Poisoning Start Hour**		(Default: 3)
		0-23
* **Opt 13: Cathode Poisoning Duration**		(Default: 1)
		1-10 *hours*
* **Opt 14: Automatic Daylight Savings Adjustment**	(Default: 1)
	At the time of writing DST in the United States is standardized to:
	‘Spring Ahead’ on the second Sunday of March at 2:00 AM
	‘Fall Back’ on the first Sunday of November at 2:00 AM
		1:*enable*
		0:*disable*
* **Opt 15: DST Spring Ahead Hour**			(Default: 2)	
	Hour to move clock forward
* **Opt 16: DST Spring Ahead Day of the Week**	(Default: 0)
		0-6 	*(0:Sunday… 6:Saturday)*
* **Opt 17: DST Spring Ahead Week**			(Default: 2)
		1-4
* **Opt 18: DST Spring Ahead Month**		(Default: 3)
		1-12
* **Opt 19: DST Fall Back Hour**			(Default: 2)
	Hour to move clock backward
* **Opt 20: DST Fall Back Day**			(Default: 0)
		0-6 	*(0:Sunday…6:Saturday)*
* **Opt 21: DST Fall Back Week**			(Default: 1)
		1-4
* **Opt 22: DST Fall Back Month**			(Default: 11)
		1-12
* **Opt 23: PWM Frequency Scaling**		
	This value is read only and is present only for debugging purposes
* **Opt 24: Software Time Correction**		(Default: 1.94)
	Add/Subtract number of seconds per week
	(Left colon illuminates for negative numbers)
	Quartz crystals like the one used for timekeeping in this clock are not 
	perfect. They inherently have some error in their resonant frequency. 
	To make matters worse, this can change as the crystal ages or experiences 
	fluxuations in temperature. If the error of the crystal is somewhat 
	constant it can be corrected for. For the DS32KHZ modules used in these 
	clocks I measured that the clock runs slow by 1.94 seconds per week, hence 
	the value is set to add 1.94. You should only change this value after 
	careful measurement over a minimum of one week. A 32.768KHz testpoint is 
	provided, or you can set the colons to .5 or 1Hz to measure from.
* **Opt 25: Day of week**
	This value is read only and is provided only as a sanity check for the DST options 			
		0:*Sunday…6:Saturday*

## General Notes:
**Use caution**, the tubes on this clock are fragile. Besides that, they run at about **180**
**volts DC**. While the current available limited you could still receive an unpleasant shock.

The power supply should be 12V DC @ 500mA center positive 2.1mm DC Barrel Jack, 
however the clock will tolerate 10-35V and is reverse polarity protected. There is a 
built in surface mount fuse (which should be socketed, but it isn’t) next to the power 
plug in case of overload. If the clock doesn’t turn on, check it for continuity. Test 
points are provided for adjustment of the HV PSU, which should be set to 180V, but use 
care if you try to adjust it!
