/* Name: main.c
  
  created by chris chung, 2010 April

  based on the works found in

  v-usb framework http://www.obdev.at/vusb/
	 Project: Thermostat based on AVR USB driver
	 Author: Christian Starkjohann
    
  usbtiny isp http://www.xs4all.nl/~dicks/avr/usbtiny/
  	Dick Streefland
  
  please observe licensing term from the above two projects

	Copyright (C) 2010  chris chung

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


  **** fuse setting, 
  **** this will blow reset fuse, u will need to use HV programmer to recover if u mess up
  avrdude -c usbtiny -p t45 -V -U lfuse:w:0xe1:m -U hfuse:w:0x5d:m -U efuse:w:0xff:m 
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>

#include "usbdrv.h"
#include "oddebug.h"

enum
{
	// Generic requests
	USBTINY_ECHO,		// echo test
	USBTINY_READ,		// read byte (wIndex:address)
	USBTINY_WRITE,		// write byte (wIndex:address, wValue:value)
	USBTINY_CLR,		// clear bit (wIndex:address, wValue:bitno)
	USBTINY_SET,		// set bit (wIndex:address, wValue:bitno)
	// Programming requests
	USBTINY_POWERUP,	// apply power (wValue:SCK-period, wIndex:RESET)
	USBTINY_POWERDOWN,	// remove power from chip
	USBTINY_SPI,		// issue SPI command (wValue:c1c0, wIndex:c3c2)
	USBTINY_POLL_BYTES,	// set poll bytes for write (wValue:p1p2)
	USBTINY_FLASH_READ,	// read flash (wIndex:address)
	USBTINY_FLASH_WRITE,	// write flash (wIndex:address, wValue:timeout)
	USBTINY_EEPROM_READ,	// read eeprom (wIndex:address)
	USBTINY_EEPROM_WRITE,	// write eeprom (wIndex:address, wValue:timeout)
};

#define	PORT	PORTB
#define	DDR		DDRB
#define	PIN		PINB

//
// to reduce pin count so that this can fit in a 8 pin tiny
// . no power nor ground pins to target, they are to be connected always
// . no reset control pin to target, target reset always grounded
//   * this had caused problem and there are two solutions
//     1. provide a toggle switch to off-on-off target reset to ground
//     2. introduce reset control and use reset pin as io
//
#define	POWER_MASK	0x00
#define	GND_MASK	0x00

#define	RESET_MASK	(1 << 5)
#define	SCK_MASK	(1 << 2)
#define	MISO_MASK	(1 << 1)
#define	MOSI_MASK	(1 << 0)

// ----------------------------------------------------------------------
// Programmer input pins:
//	MISO	PD3	(ACK)
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Local data
// ----------------------------------------------------------------------
static	uchar		sck_period=1;	// SCK period in microseconds (1..250)
static	uchar		poll1;		// first poll byte for write
static	uchar		poll2;		// second poll byte for write
static	unsigned		address;	// read/write address
static	unsigned		timeout;	// write timeout in usec
static	uchar		cmd0;		// current read/write command byte
static	uchar		cmd[4];		// SPI command buffer
static	uchar		res[4];		// SPI result buffer

// ----------------------------------------------------------------------
// Delay exactly <sck_period> times 0.5 microseconds (6 cycles).
// ----------------------------------------------------------------------
__attribute__((always_inline))
static	void	delay ( void )
{
	asm volatile(
		"	mov	__tmp_reg__,%0	\n"
		"0:	rjmp	1f		\n"
		"1:	nop			\n"
		"2:	nop			\n"
		"3:	nop			\n"
		"	dec	__tmp_reg__	\n"
		"	brne	0b		\n"
		: : "r" (sck_period) );
}

// ----------------------------------------------------------------------
// Issue one SPI command.
// ----------------------------------------------------------------------
static	void	spi ( uchar* cmd, uchar* res )
{
	uchar	i;
	uchar	c;
	uchar	r;
	uchar	mask;

	for	( i = 0; i < 4; i++ )
	{
		c = *cmd++;
		r = 0;
		for	( mask = 0x80; mask; mask >>= 1 )
		{
			if	( c & mask )
			{
				PORT |= MOSI_MASK;
			}
			delay();
			PORT |= SCK_MASK;
			delay();
			r <<= 1;
			if	( PIN & MISO_MASK )
			{
				r++;
			}
			PORT &= ~MOSI_MASK;
			PORT &= ~SCK_MASK;
		}
		*res++ = r;
	}
}

// ----------------------------------------------------------------------
// Create and issue a read or write SPI command.
// ----------------------------------------------------------------------
static	void	spi_rw ( void )
{
	unsigned	a;

	a = address++;
	if	( cmd0 & 0x80 )
	{	// eeprom
		a <<= 1;
	}
	cmd[0] = cmd0;
	if	( a & 1 )
	{
		cmd[0] |= 0x08;
	}
	cmd[1] = a >> 9;
	cmd[2] = a >> 1;
	spi( cmd, res );
}

// ----------------------------------------------------------------------
// Handle an IN packet.
// ----------------------------------------------------------------------
uchar usbFunctionRead(uchar *data, uchar len)
{
	uchar	i;

	for	( i = 0; i < len; i++ )
	{
		spi_rw();
		data[i] = res[3];
	}
	return len;
}

// ----------------------------------------------------------------------
// Handle an OUT packet.
// ----------------------------------------------------------------------
uchar usbFunctionWrite(uchar *data, uchar len)
{
	uchar	i;
	unsigned	usec;
	uchar	r;
	//uchar	last = (len != 8);

	for	( i = 0; i < len; i++ )
	{
		cmd[3] = data[i];
		spi_rw();
		cmd[0] ^= 0x60;	// turn write into read
		//
		for	( usec = 0; usec < timeout; usec += 32 * sck_period )
		{	// when timeout > 0, poll until byte is written
			spi( cmd, res );
			r = res[3];
			if	( r == cmd[3] && r != poll1 && r != poll2 )
			{
				break;
			}
		}
		//
	}
	//return last;
	return 1;
}

/* ------------------------------------------------------------------------- */
/* ------------------------ interface to USB driver ------------------------ */
/* ------------------------------------------------------------------------- */

uchar	usbFunctionSetup(uchar data[8])
{
// ----------------------------------------------------------------------
// Handle a non-standard SETUP packet.
// ----------------------------------------------------------------------
	uchar	bit;
	uchar	mask;
	uchar*	addr;
	uchar	req;

	// Generic requests
	req = data[1];
	if	( req == USBTINY_ECHO )
	{
		usbMsgPtr = data;
		return 8;
	}
	addr = (uchar*) (int) data[4];
	if	( req == USBTINY_READ )
	{
		data[0] = *addr;
		usbMsgPtr = data;
		return 1;
	}
	if	( req == USBTINY_WRITE )
	{
		*addr = data[2];
		return 0;
	}
	bit = data[2] & 7;
	mask = 1 << bit;
	if	( req == USBTINY_CLR )
	{
		*addr &= ~ mask;
		return 0;
	}
	if	( req == USBTINY_SET )
	{
		*addr |= mask;
		return 0;
	}

	// Programming requests
	if	( req == USBTINY_POWERUP )
	{
		//sck_period = data[2];
		if (data[2] == 250)
		  sck_period = 250;
    else
  		sck_period = 1;

		mask = POWER_MASK;
		if	( data[4] )
		{
			mask |= RESET_MASK;
		}
		DDR  &= ~MISO_MASK;
		DDR  |= (RESET_MASK|SCK_MASK|MOSI_MASK);
		PORT &= ~(RESET_MASK|SCK_MASK|MOSI_MASK|MISO_MASK);
		return 0;
	}
	if	( req == USBTINY_POWERDOWN )
	{
		//PORT |= RESET_MASK;
		//DDR  &= ~(SCK_MASK|MOSI_MASK);
		DDRB  = RESET_MASK;
		PORTB = RESET_MASK;
		return 0;
	}
	/* have to remove the following check as we strip a lot of io
	if	( ! PORT )
	{
		return 0;
	}
	*/
	if	( req == USBTINY_SPI )
	{
		spi( data + 2, data + 0 );
		usbMsgPtr = data;
		return 4;
	}
	if	( req == USBTINY_POLL_BYTES )
	{
		poll1 = data[2];
		poll2 = data[3];
		return 0;
	}
	address = * (unsigned*) & data[4];
	if	( req == USBTINY_FLASH_READ )
	{
		cmd0 = 0x20;
		return 0xff;	// usb_in() will be called to get the data
	}
	if	( req == USBTINY_EEPROM_READ )
	{
		cmd0 = 0xa0;
		return 0xff;	// usb_in() will be called to get the data
	}
	timeout = * (unsigned*) & data[2];
	if	( req == USBTINY_FLASH_WRITE )
	{
		cmd0 = 0x40;
		return 0xff;	// data will be received by usb_out()
	}
	if	( req == USBTINY_EEPROM_WRITE )
	{
		cmd0 = 0xc0;
		return 0xff;	// data will be received by usb_out()
	}
	return 0;
}


/* ------------------------------------------------------------------------- */
/* ------------------------ Oscillator Calibration ------------------------- */
/* ------------------------------------------------------------------------- */

/* Calibrate the RC oscillator to 8.25 MHz. The core clock of 16.5 MHz is
 * derived from the 66 MHz peripheral clock by dividing. Our timing reference
 * is the Start Of Frame signal (a single SE0 bit) available immediately after
 * a USB RESET. We first do a binary search for the OSCCAL value and then
 * optimize this value with a neighboorhod search.
 * This algorithm may also be used to calibrate the RC oscillator directly to
 * 12 MHz (no PLL involved, can therefore be used on almost ALL AVRs), but this
 * is wide outside the spec for the OSCCAL value and the required precision for
 * the 12 MHz clock! Use the RC oscillator calibrated to 12 MHz for
 * experimental purposes only!
 */
static void calibrateOscillator(void)
{
uchar       step = 128;
uchar       trialValue = 0, optimumValue;
int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

    /* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    /* proportional to current real frequency */
        if(x < targetValue)             /* frequency still too low */
            trialValue += step;
        step >>= 1;
    }while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; /* this is certainly far away from optimum */
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
}
/*
Note: This calibration algorithm may try OSCCAL values of up to 192 even if
the optimum value is far below 192. It may therefore exceed the allowed clock
frequency of the CPU in low voltage designs!
You may replace this search algorithm with any other algorithm you like if
you have additional constraints such as a maximum CPU clock.
For version 5.x RC oscillators (those with a split range of 2x128 steps, e.g.
ATTiny25, ATTiny45, ATTiny85), it may be useful to search for the optimum in
both regions.
*/

void    usbEventResetReady(void)
{
    calibrateOscillator();
    eeprom_write_byte(0, OSCCAL);   /* store the calibrated value in EEPROM */
}

/* ------------------------------------------------------------------------- */
/* --------------------------------- main ---------------------------------- */
/* ------------------------------------------------------------------------- */

int main(void) {
	uchar   i;
	uchar   calibrationValue;

	//DDRB  = (RESET_MASK|SCK_MASK|MOSI_MASK);
	DDRB  = RESET_MASK;
	PORTB = RESET_MASK;
	/*
	_delay_ms(25);
	uchar pgm[] = { 0xac, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
	spi(pgm, pgm+4);
	*/
    calibrationValue = eeprom_read_byte(0); /* calibration value from last time */
    if(calibrationValue != 0xff){
        OSCCAL = calibrationValue;
    }
    odDebugInit();
    usbDeviceDisconnect();
    for(i=0;i<20;i++){  /* 300 ms disconnect */
        _delay_ms(15);
    }
    usbDeviceConnect();

    wdt_enable(WDTO_1S);

    usbInit();
    sei();
    for(;;){    /* main event loop */
        wdt_reset();
        usbPoll();
    }
    return 0;
}
