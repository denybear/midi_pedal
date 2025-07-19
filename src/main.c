/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"			// in pico_examples git

#include "bsp/board_api.h"
#include "tusb.h"


/* This part is the Neopixel part. Upon receiving a MIDI note-on, we light the leds of Neopixel
 * and we keep these lit for a few ms
 */

#define LED_PIN		6	// GPIO 6 (pin #9) connected to NeoPixel data line
#define NUM_PIXELS	1	// Number of NeoPixels in the strip
// globals
PIO pio = pio0;
uint sm = 0;
uint64_t neoPixelOnTime = 0;			// current time when Neopixel has been lit 
int neoPixelState = 3;					// determine whether Neopixel strip should be set to on
										// 0: light in black
										// 1: light in red
										// 2: light in yellow
										// 3: do nothing (pass)
	
void set_pixel_color(uint32_t color, uint32_t pixel_index, uint32_t *pixels) {
    pixels [pixel_index] = color;
}

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
}

void light_strip (uint32_t color) {

	uint32_t pixels[NUM_PIXELS] = {0};

	for (uint32_t i = 0; i < NUM_PIXELS; i++) {
		set_pixel_color (color, i, pixels);
		for (int j = 0; j < NUM_PIXELS; j++) {
			pio_sm_put_blocking (pio, sm, pixels [j] << 8u);	// Send data to NeoPixels
		}
	}
}
/* End of neopixel part
 */



/* This MIDI example sends midi note-on and note-off commands at press of a switch.
 * There are 5 switches on my home-made pedal; a different note is assigned to each
 * - Linux (Ubuntu): install qsynth, qjackctl. Then connect TinyUSB output port to FLUID Synth input port
 * - Windows: install MIDI-OX
 * - MacOS: SimpleSynth
 */

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

// MIDI constants
#define MIDI_NOTEON	0x90
#define CHANNEL		0     // midi channel 1

// midi notes corresponding to foot pedal switches
#define NOTE_SW1	0
#define NOTE_SW2	1
#define NOTE_SW3	2
#define NOTE_SW4	3
#define NOTE_SW5	4
// midi notes corresponding to finger switches
#define NOTE_SW6	5
#define NOTE_SW7	6
#define NOTE_SW8	7


// pedal GPIO
#define SWITCH_1	11
#define SWITCH_2	12
#define SWITCH_3	13
#define SWITCH_4	14
#define SWITCH_5	15
// finger GPIO
#define SWITCH_6	10
#define SWITCH_7	8
#define SWITCH_8	9


// switches stored as bit table
#define SW1			1
#define SW2			2
#define SW3			4
#define SW4			8
#define SW5			16
#define SW6			32
#define SW7			64
#define SW8			128


// type definition
struct pedalboard {
	int value;				// value of pedal variable at the time of calling the function: describes which pedal is pressed
	bool change_state;		// describes whether pedal state has changed from last call
	int change_value;		// describes pedal value when state is changed
	uint64_t change_time;	// describes time elapsed between previous state change and current state change (ie. between previous press and current press); 0 if no state change
};

// function prototypes
void midi_task(struct pedalboard *);
int test_switch (int , struct pedalboard*);


/*------------- MAIN -------------*/
int main(void)
{
	struct pedalboard pedal;

	stdio_init_all();
	board_init();
	printf("MIDI-pedal\r\n");

	// init device stack on configured roothub port
	tud_init(BOARD_TUD_RHPORT);

	if (board_init_after_tusb) {
		board_init_after_tusb();
	}

	gpio_init(SWITCH_1);
	gpio_set_dir(SWITCH_1, GPIO_IN);
	gpio_pull_up (SWITCH_1);		 // switch pull-up

	gpio_init(SWITCH_2);
	gpio_set_dir(SWITCH_2, GPIO_IN);
	gpio_pull_up (SWITCH_2);		 // switch pull-up

	gpio_init(SWITCH_3);
	gpio_set_dir(SWITCH_3, GPIO_IN);
	gpio_pull_up (SWITCH_3);		 // switch pull-up

	gpio_init(SWITCH_4);
	gpio_set_dir(SWITCH_4, GPIO_IN);
	gpio_pull_up (SWITCH_4);		 // switch pull-up

	gpio_init(SWITCH_5);
	gpio_set_dir(SWITCH_5, GPIO_IN);
	gpio_pull_up (SWITCH_5);		 // switch pull-up

	gpio_init(SWITCH_6);
	gpio_set_dir(SWITCH_6, GPIO_IN);
	gpio_pull_up (SWITCH_6);		 // switch pull-up

	gpio_init(SWITCH_7);
	gpio_set_dir(SWITCH_7, GPIO_IN);
	gpio_pull_up (SWITCH_7);		 // switch pull-up

	gpio_init(SWITCH_8);
	gpio_set_dir(SWITCH_8, GPIO_IN);
	gpio_pull_up (SWITCH_8);		 // switch pull-up

	// init pedal structure to all 0
	pedal.value = 0;
	pedal.change_state = false;
	pedal.change_value = 0;
	pedal.change_time = 0;

	// Neopixels inits
	// Initialize PIO and load the WS2812 program
	uint offset = (uint) pio_add_program (pio, &ws2812_program);
	ws2812_program_init (pio, sm, offset, LED_PIN, 800000, false);
	// End of NeoPixel inits


	// main
	while (1) {
		tud_task(); 		// tinyusb device task
		midi_task(&pedal);	// manage midi tasks



		light_strip (rgb_to_color (255, 0, 0));	// red
		sleep_ms (100);




		// manage neopixel led strip: light the strip with the right color; in case of black, wait 150ms before unlighting
		switch (neoPixelState) {
			case 0:
				if (((to_us_since_boot (get_absolute_time()) - neoPixelOnTime)) > 150000) {	// paint to black after 150ms
					light_strip (rgb_to_color (0, 0, 0));	// black
					neoPixelState = 3;						// next time do nothing
				}
				break;
			case 1:
				neoPixelOnTime = to_us_since_boot (get_absolute_time());
				light_strip (rgb_to_color (255, 0, 0));	// red
				neoPixelState = 0;							// next state is black
				break;
			case 2:
				neoPixelOnTime = to_us_since_boot (get_absolute_time());
				light_strip (rgb_to_color (255, 255, 0));	// yellow
				neoPixelState = 0;							// next state is black
				break;
			default:
				break;
		}
		// end of management of neopixel led strip	
	}
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
	//blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
	//blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
	(void) remote_wakeup_en;
	//blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
	//blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// MIDI Task
//--------------------------------------------------------------------+

void midi_task(struct pedalboard *pd)
{
	uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint


	// The MIDI interface always creates input and output port/jack descriptors
	// regardless of these being used or not. Therefore incoming traffic should be read
	// (possibly just discarded) to avoid the sender blocking in IO
	// here, we check for note_on event, and if received, then we light the Neopixel strip
	uint8_t packet[4];

	while ( tud_midi_available() ) {
		tud_midi_packet_read (packet);

		// NeoPixel part
		// test if note on, and velocity not null: in this case, lite the leds ON (in the while loop)
		if ((packet [0] == (MIDI_NOTEON | CHANNEL)) && (packet [2] != 0)) {
			if (packet [1] == 0) neoPixelState = 1;		// first beat (note_on == 0) is red
			else neoPixelState = 2;						// other beats (other note_ons) are yellow
        }
		// End of Neopixel part
    }


	// test pedal and check if one of them is pressed
	test_switch (SW1 | SW2 | SW3 | SW4 | SW5 | SW6 | SW7 | SW8, pd);

	// check if state has changed, ie. pedal has just been pressed or unpressed
	if (pd->change_state) {

		uint8_t note_on[3] = { MIDI_NOTEON | CHANNEL, 0, 127 };

		if (pd->value & SW1) {
			// Send Note On for current switch at full velocity (127) on channel.
			note_on[1] = NOTE_SW1;
			tud_midi_stream_write(cable_num, note_on, 3);
		}
		if (pd->value & SW2) {
			// Send Note On for current switch at full velocity (127) on channel.
			note_on[1] = NOTE_SW2;
			tud_midi_stream_write(cable_num, note_on, 3);
		}
		if (pd->value & SW3) {
			// Send Note On for current switch at full velocity (127) on channel.
			note_on[1] = NOTE_SW3;
			tud_midi_stream_write(cable_num, note_on, 3);
		}
		if (pd->value & SW4) {
			// Send Note On for current switch at full velocity (127) on channel.
			note_on[1] = NOTE_SW4;
			tud_midi_stream_write(cable_num, note_on, 3);
		}
		if (pd->value & SW5) {
		// Send Note On for current switch at full velocity (127) on channel.
			note_on[1] = NOTE_SW5;
			tud_midi_stream_write(cable_num, note_on, 3);
		}
		if (pd->value & SW6) {
		// Send Note On for current switch at full velocity (127) on channel.
			note_on[1] = NOTE_SW6;
			tud_midi_stream_write(cable_num, note_on, 3);
		}
		if (pd->value & SW7) {
		// Send Note On for current switch at full velocity (127) on channel.
			note_on[1] = NOTE_SW7;
			tud_midi_stream_write(cable_num, note_on, 3);
		}
		if (pd->value & SW8) {
		// Send Note On for current switch at full velocity (127) on channel.
			note_on[1] = NOTE_SW8;
			tud_midi_stream_write(cable_num, note_on, 3);
		}
	}
}

//--------------------------------------------------------------------+
// SWITCH PRESS DETECTION
//--------------------------------------------------------------------+
// test switches and return which switch has been pressed (FALSE if none)
int test_switch (int pedal_to_check, struct pedalboard* pedal)
{
	int result = 0;
	static int previous_result = 0;							// previous value for result, required for anti-bounce; this MUST BE static
	static uint64_t this_press, previous_press = 0;			// time between 2 state changes; this MUST be static
	int i;


	// by default, we assume there is no change in the pedal state (ie. same pedals are pressed / unpressed as for previous function call)
	pedal->change_state = false;

	// determine for how long we are in the current state
	this_press = to_us_since_boot (get_absolute_time());
	pedal->change_time = this_press - previous_press;

	// test if switch has been pressed
	// in this case, line is down (level 0)
	if ((pedal_to_check & SW1) && gpio_get (SWITCH_1)==0) {
		result |= SW1;
	}

	if ((pedal_to_check & SW2) && gpio_get (SWITCH_2)==0) {
		result |= SW2;
	}

	if ((pedal_to_check & SW3) && gpio_get (SWITCH_3)==0) {
		result |= SW3;
	}

	if ((pedal_to_check & SW4) && gpio_get (SWITCH_4)==0) {
		result |= SW4;
	}

	if ((pedal_to_check & SW5) && gpio_get (SWITCH_5)==0) {
		result |= SW5;
	}

	if ((pedal_to_check & SW6) && gpio_get (SWITCH_6)==0) {
		result |= SW6;
	}

	if ((pedal_to_check & SW7) && gpio_get (SWITCH_7)==0) {
		result |= SW7;
	}

	if ((pedal_to_check & SW8) && gpio_get (SWITCH_8)==0) {
		result |= SW8;
	}

	// LED ON or LED OFF depending if a switch has been pressed
	board_led_write(result ? true : false);

	// check whether there has been a change of state in the pedal (pedal pressed or unpressed...)
	// this allows to have anti-bouncing when pedal goes from unpressed to pressed, or from pressed to unpressed
	if (result != previous_result) {
		// pedal state has changed; set variables accordingly
		pedal->change_state = true;
		pedal->change_value = previous_result;
		previous_press = this_press;

		// anti-bounce of 50ms
		for (i = 0; i < 50; i++) {
			// wait 1ms: not sure whether sleep or busy_wait are blocking background threads
			sleep_ms (1);
		}
	}

	// copy pedal values and return
	previous_result = result;
	pedal->value = result;
	return result;
}

