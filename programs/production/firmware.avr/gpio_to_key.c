/*
 * Firmware for the AVR teensey.   Translates GPIO pins into
 * character. 
 *
 * B pins go to A-H.
 * D pins go to a-h.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard.h"

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

int main(void)
{
    uint8_t b;		  // Data from GPIO B register
    uint8_t d;		  // Data from GPIO B register
    uint8_t b_prev=0xFF;  // Last data from the B register
    uint8_t d_prev=0xFF;  // Last data from the D register
    uint8_t mask;	  // Current idle bitmask
    uint8_t i;		  // Loop counter

    // set for 16 MHz clock
    CPU_PRESCALE(0);

    // Configure all port B and port D pins as inputs with pullup resistors.
    // See the "Using I/O Pins" page for details.
    // http://www.pjrc.com/teensy/pins.html
    DDRD = 0x00;
    DDRB = 0x00;
    PORTB = 0xFF;
    PORTD = 0xFF;

    // Initialize the USB, and then wait for the host to set configuration.
    // If the Teensy is powered without a PC connected to the USB port,
    // this will wait forever.
    usb_init();
    while (!usb_configured()) 
	continue/* wait */ ;

    // Wait an extra second for the PC's operating system to load drivers
    // and do whatever it does to actually be ready for input
    _delay_ms(1000);

    while (1) {
	// read all port B and port D pins
	b = PINB;
	d = PIND;
	// check if any pins are low, but were high previously
	
	mask = 1;
	for (i=0; i<8; i++) {
	    if (((b & mask) == 0) && (b_prev & mask) != 0) {
		usb_keyboard_press(KEY_A+i, 0);
	    }
	    if (((d & mask) == 0) && (d_prev & mask) != 0) {
		usb_keyboard_press(KEY_M+i, 0);
	    }
	    mask = mask << 1;
	}
	// now the current pins will be the previous, and
	// wait a short delay so we're not highly sensitive
	// to mechanical "bounce".
	b_prev = b;
	d_prev = d;
	_delay_ms(2);
    }
}


