/*
 * button_avr -- Lock on to the AVR teensey keyboard device and send
 * 	button codes to the garden pipeline
 *
 * Note: Because of the way the teensy is laid out we have the following
 * key->button mapping
 * 
 * Pin	  Signal  Key  Code
 * Pin 1  GND     --   --
 * Pin 2  B0	  A    0
 * Pin 3  B1	  B    1
 * Pin 4  B2	  C    2
 * Pin 5  B3	  D    3
 * Pin 6  B7	  H    4
 * Pin 7  D0	  M    5
 * Pin 8  D1	  N    6
 * Pin 9  D2	  O    7
 * Pin 10 D3	  P    8
 */
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <syslog.h>

#include "device.h"

/*
 * grab_dev -- Get exclusive use of the device
 */
static void grab_dev(const int fd) 
{
    if (ioctl(fd, EVIOCGRAB, (void *)1) != 0) {
	std::cout << "Grab failed for fd=" << fd << " aborting" << std::endl;
	exit(8);
    }
}
/*
 * get_key -- Read a key from a keyboard
 *
 * Parameters
 * 	fd -- FD of the keyboard
 */
static int get_key(const int fd)
{
    while (true) {
	// Event we read from the raw event system
	struct input_event event;

	// Read event, get size
	int read_size = read(fd, &event, sizeof(event));
	if (read_size <= 0) {
	    std::cout <<  "Read error when reading event from " << fd << std::endl;
	    return (-1);
	}
	if ((event.type == EV_KEY) && (event.value == 1)) {
	    // 69 is some sort of extended code
	    if (event.code == 69)
		continue;
	    return (event.code);
	}
    }
}
/*
 * generic_input -- Read an input device and handle the key presses
 *
 * Parameters
 * 	device -- Device to read
 */
static void generic_input(const char* const device)
{
    int out_fd;	// FD for the output fifo

    while (true) {
	// Open the socket for this process
	out_fd = open(INPUT_PIPE, O_WRONLY);
	if (out_fd < 0) {
	    syslog(LOG_ERR, "ERROR: Could not open input pipe");
	    sleep(30);
	} else {
	    break;
	}
    }

    while (true) {
	int fd;	// The fd of the device

	// Loop until we get the device
	while (true) {
	    // Get the device
	    fd = open(device, O_RDONLY);
	    if (fd >= 0) {
		break;
	    }
	    syslog(LOG_ERR, "ERROR: Could not open %s -- sleeping", device);
	    sleep(10);
	    syslog(LOG_ERR, "Sleep done ");
	}
	grab_dev(fd);

	while (true) {
	    // Get key code
	    int key = get_key(fd);
	    syslog(LOG_INFO, "Key %d pressed", key);

	    if (key < 0) {
		close(fd);
		break;
	    }

	    switch (key) {
		case KEY_A:
		    write(out_fd, "0", 1);
		    break;
		case KEY_B:
		    write(out_fd, "1", 1);
		    break;
		case KEY_C:
		    write(out_fd, "2", 1);
		    break;
		case KEY_D:
		    write(out_fd, "3", 1);
		    break;
		case KEY_H:
		    write(out_fd, "4", 1);
		    break;
		case KEY_M:
		    write(out_fd, "5", 1);
		    break;
		case KEY_N:
		    write(out_fd, "6", 1);
		    break;
		case KEY_O:
		    write(out_fd, "7", 1);
		    break;
		case KEY_P:
		    write(out_fd, "8", 1);
		    break;
		default:
		    break;
	    }
	}
    }
}

int main(int argc, char* argv[])
{
    bool stdout_log = false;	// Send log messages to stdout

    int opt;	// Option we are looking
    while ((opt = getopt(argc, argv, "s")) != -1) {
	switch (opt) {
	    case 's':
		stdout_log = true;
		break;
	    default:
		std::cerr << "Usage " << argv[0] << " [-s]" << std::endl;
		exit(EXIT_FAILURE);
	}
    }
    if (optind < argc) {
	std::cerr << "Extra arguements on the command line" << std::endl;
	exit(EXIT_FAILURE);
    }
    // Open up the syslog system
    openlog("button_avr", stdout_log ? LOG_PERROR : 0, LOG_USER); 

    generic_input(AVR_KBD);
    return (0);
}
