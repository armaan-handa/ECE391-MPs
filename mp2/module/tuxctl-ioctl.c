/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

#define bit_mask_4	0x0F
#define CLEAR_LED_STATE 0xF0000


// function declarations
void tux_init (struct tty_struct*);
int tux_buttons (struct tty_struct*, unsigned long);
void tux_set_led (struct tty_struct*, unsigned long);
void clear_LED(struct tty_struct*);


spinlock_t lock;				// spin lock for global variables
uint8_t button;					// byte that holds which buttons are pressed. R D L U C B A S
static unsigned long led_state; // save state of LEDs
uint8_t _reset;					// flag to check if device is coming out of reset

// table with hex values to select which LEDs to light up for digit
uint8_t led_table[16] = {
	0xE7, 0x06, 0xCB, 0x8F,
	0x2E, 0xAD, 0xED, 0x86,
	0xEF, 0xAE, 0xEE, 0x6D,
	0xE1, 0x4F, 0xE9, 0xE8
};


/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;
	uint8_t buf[2];

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

	switch (a)
	{
	case MTCP_BIOC_EVENT: // handle bioc
		b &= bit_mask_4;	// 4 LSBs are which of C, B, A and Start are pressed
		c &= bit_mask_4;	// 4 LSBs are which of Right, Down, Left and Up are pressed
		c = c << 4;			// move C to 4 MSB
		if(!spin_is_locked(&lock))
		{
			button = b | c;		// Store which buttons are pressed
		}
		break;
	case MTCP_ACK:
		// handle ack
		if (_reset != 0)
		{
			_reset = 0;
		}
		break;
	case MTCP_RESET: //handle reset
		spin_lock(&lock); 
		_reset = 1; // set reset flag
		buf[0] = MTCP_LED_USR; // reinitialize LED USR MODE
		buf[1] = MTCP_BIOC_ON; // reinitialize BIOC
		tuxctl_ldisc_put(tty, buf, 2); // write buffer to device
		tux_set_led(tty, led_state); // set leds back to what they were prior to reset
		spin_unlock(&lock);
		break;
	default:
		break;
	}

    /*printk("packet : %x %x %x\n", a, b, c); */
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/

/*
 * tuxctl_ioctl
 * 	dispatcher for tux controller ioctls
 * 	INPUTS: tty -- tty structure to communicate with board
 * 			cmd -- which ioctl to call
 * 			arg -- argument to pass into ioctl
 *  OUTPUTS: none
 *  RETURN VALUE: 0 for success, -EINVAL  if cmd is invalid
 */
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:
			tux_init(tty);
			return 0;
	case TUX_BUTTONS:
		if (arg == 0) // if argument is invalid, function will return
		{
			return -EINVAL;
		}
			return tux_buttons(tty, arg);
	case TUX_SET_LED:
			tux_set_led(tty, arg);
			return 0;
	case TUX_LED_ACK:
	case TUX_LED_REQUEST:
	case TUX_READ_LED:
	default:
	    return -EINVAL;
    }
}

/*
 * tux_init
 * 	ioctl[ to initializze tux controller
 * 	INPUTS: tty -- tty structure to communicate with board
 *  OUTPUTS: none
 *  RETURN VALUE: none
 * 	SIDE EFFECTS: initializes tux controller, sets LED to user mode, turn on BIOC, clears LED state
 */

void tux_init (struct tty_struct* tty)
{
	// printk("init tux\n");
	uint8_t buf[2]; // bbuffer to write to device
	spin_lock_init(&lock);

	button = 0x00; // init button state
	led_state = CLEAR_LED_STATE; // init led state

	buf[0] = MTCP_LED_USR; // set LED mode to USR
	buf[1] = MTCP_BIOC_ON; // enable BIOC
	tuxctl_ldisc_put(tty, buf, 2); // write buffer to device

	clear_LED(tty); // clear LEDs
}


/*
 * tux_buttons
 * 	copies state of buttons to userspace
 * 	INPUTS: tty -- tty structure to communicate with board
 * 			arg -- pointer to where button state has to be copied
 *  OUTPUTS: none
 *  RETURN VALUE: 0 for success, -EINVAL  if arg is invalid
 */
int tux_buttons (struct tty_struct* tty, unsigned long arg)
{
	int not_valid; 
	spin_lock(&lock);
	not_valid = copy_to_user((int*)arg, &button, 1); // copy button to user space
	spin_unlock(&lock);
	if (not_valid)
	{
		return -EINVAL; // if copy failed, return -EINVAL
	}
	return 0; // return 0 for success
}


/*
 * tux_set_led
 * 	sets the value that needs to be displayed on LED display, including decimal points
 * 	INPUTS: tty -- tty structure to communicate with board
 * 			arg -- 32 bit integer containing information on which leds need to be turned on, which decimal points to display, and what value to display
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: displays value on LEDs. Will remove whatever was previously being displayed. Will change led_state
 */
void tux_set_led (struct tty_struct* tty, unsigned long arg)
{
	// printk("setting leds\n");
	uint16_t val = arg & 0xFFFF; // hex value to be printed
	uint8_t digits = (arg >> 16) & 0xF; // which digits need to be turned on 
	uint8_t decimals = (arg >> 24) & 0xF; // which decimal points to show
	uint8_t hex[4]; // array of hex digits
	uint8_t segments[4]; // array containing what leds to turn on in seven segment display
	uint8_t leds[4]; // array containing information about which digits need to be turned on
	uint8_t decs[4]; // array containing information about which decimal points to display
	uint8_t buf[6]; // buffer to write to dcevice
	int i; // loop index
	int index = 2; // buffer index

	// printk("val: %d, arg: %d\n", val, arg);

	spin_lock(&lock);
	led_state = arg; // save new state of leds incase of reset
	spin_unlock(&lock);

	clear_LED(tty);

	hex[0] = (val & 0xF); // set hex val of digits
	hex[1] = (val & 0xF0) >> 4;
	hex[2] = (val & 0xF00) >> 8;
	hex[3] = val >> 12;

	leds[0] = digits & 0x1; // select which leds to turn on
	leds[1] = (digits & 0x2) >> 1;
	leds[2] = (digits & 0x4) >> 2;
	leds[3] = digits >> 3;

	decs[0] = (decimals & 0x1); // select which decimals to turn on 
	decs[1] = (decimals & 0x2) >> 1;
	decs[2] = (decimals & 0x4) >> 2;
	decs[3] = decimals >> 3;
	
	buf[0] = MTCP_LED_SET; // put LED_SET opcode in buffer
	buf[1] = digits; // set which digits to turn on in buffer

	for (i = 0; i < 4; i++)
	{
		segments[i] = led_table[hex[i]]; // display hex value on seven segment
		segments[i] |= decs[i] << 4; // choose whether to turn on that decimal point
		if (leds[i] == 1)
		{
			buf[index] = segments[i]; // set seven segment value to buffer
			index++;
		}
	}
	
	tuxctl_ldisc_put(tty, buf, index); // copy buffer to device
}

/*
 * tux_set_led
 *  clears LEDs on tux controller
 * 	INPUTS: tty -- tty structure to communicate with board
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: will remove whatever is being displayed on LEDs
 */
void clear_LED(struct tty_struct* tty)
{
	uint8_t buf[6]; // buffer to write
	buf[0] = MTCP_LED_SET; // put LED_SET opcode in buffer
	buf[1] = 0xF; // select all 4 digits
	buf[2] = 0; // set digits to blank
	buf[3] = 0;
	buf[4] = 0;
	buf[5] = 0;

	tuxctl_ldisc_put(tty, buf, 6); // write buffer to device
}
