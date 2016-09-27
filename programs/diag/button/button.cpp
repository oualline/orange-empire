#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>

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
    while (true) {
	int fd;	// The fd of the device

	// Loop until we get the device
	while (true) {
	    // Get the device
	    fd = open(device, O_RDONLY);
	    if (fd >= 0) {
		break;
	    }
	    std::cout << "ERROR: Could not open " << device << " -- sleeping" << std::endl;
	    sleep(10);
	    std::cout << "Sleep done " << std::endl;
	}
	grab_dev(fd);

	while (true) {
	    // Get key code
	    int key = get_key(fd);
	    std::cout << "Key " << key << " pressed" << std::endl;
	    if (key < 0) {
		close(fd);
		break;
	    }
	}
    }
}

int main()
{
    generic_input(PROKEY55);
    return (0);
}
