#undef RELAY_DEBUG
#include <string>
#include <iostream>
#include <sstream>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h> // needed for memset
#include <poll.h>
#include <syslog.h>
#include <pthread.h>

#include "relay.h"
 
//#define RELAY_DEVICE "/dev/ttyACM0"
#define RELAY_DEVICE1 "/dev/serial/by-id/usb-Microchip_Technology_Inc._CDC_RS-232_Emulation_Demo-if00"
#define RELAY_DEVICE2 "/dev/serial/by-id/usb-Numato_Systems_Pvt._Ltd._Numato_Lab_16_Channel_USB_Relay_Module-if00"
#define RELAY_DEVICE3 "/dev/serial/by-id/usb-Numato_Systems_Pvt._Ltd._Numato_Lab_2_Channel_USB_Powered_Relay_Module-if00"
static int relay_fd = -1;	// Relay fd

// Mutex so we do access one operation at a time
static pthread_mutex_t relay_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 
 * relay_lock -- Lock the relay system
 */
static inline void relay_lock()
{
    if (pthread_mutex_lock(&relay_mutex) != 0) 
	throw relay_error("Could not lock relay system");
}
/* 
 * relay_unlock -- Unlock the relay system
 */
static inline void relay_unlock()
{
    if (pthread_mutex_unlock(&relay_mutex) != 0) 
	throw relay_error("Could not unlock relay system");
}
/*
 * read_ch -- Read a single character from the relay
 * 
 * Returns
 *	The character read
 *
 * Throws
 * 	relay_error if no response within a given time
 */
static char read_ch(void)
{
    char ch;	// Character from the relay

    // The inforation for the poll
    struct pollfd poll_in[] = {
	    { relay_fd, POLLIN, 0}
    };
    struct timespec timeout = {1, 500000000};	// Timeout is 1/2 second
    if (ppoll(poll_in, 1, &timeout, NULL) <= 0)
	throw(relay_error("Timeout"));

    if (read(relay_fd, &ch, 1) != 1) 
	throw(relay_error("Read error"));
    return (ch);
}
/*
 * raw_relay_send -- Send a command to the relay
 */
static void raw_relay_send(const std::string& cmd)
{
    std::string full_cmd = cmd + "\r";	// The complete command to send 
    if (write(relay_fd, full_cmd.c_str(), full_cmd.length()) != 
	    static_cast<ssize_t>(full_cmd.length()))
	throw(relay_error("Unable to write to device"));

#ifdef RELAY_DEBUG
    std::cout << "RELAY OUT: " << cmd << std::endl;
#endif // RELAY_DEBUG
    for (unsigned int i = 0; i < cmd.length(); ++i) {
	char ch = read_ch();	// Character that's part of the response

	if (ch != cmd.at(i)) 
	    throw(relay_error("Echo error"));
    }
    char ch = read_ch();
    if (ch != '\n') 
	throw(relay_error("Echo linefeed error"));

    ch = read_ch();
    if (ch != '\r') 
	throw(relay_error("Echo return error"));
}
/*
 * raw_relay -- Do a relay command directly to the device
 */
static void raw_relay(const std::string& cmd)
{
    if (simulate) {
	std::cout << "RAW RELAY: " << cmd << std::endl;
	return;
    }
    relay_lock();
    raw_relay_send(cmd);

    // Get the character that's a response (should be prompt)
    char ch = read_ch();
    if (ch != '>')
	throw(relay_error("Prompt response error"));
    relay_unlock();
}
/*
 * raw_relay -- Do a relay command directly to the device
 */
static std::string raw_relay_response(const std::string& cmd)
{
    relay_lock();
    raw_relay_send(cmd);

    std::string result;		// Response we expect
    while (true) {
	// Get a response character
	char ch = read_ch();

	// Check for end dance
	if (ch == '\n') {
	    ch = read_ch();
	    if (ch != '\r')
		throw(relay_error("Line feed response error"));
	    ch = read_ch();
	    if (ch != '>')
		throw(relay_error("Prompt response error"));
	    relay_unlock();
#ifdef RELAY_DEBUG
	    std::cout << "RELAY RES: " << result << std::endl;
#endif // RELAY_DEBUG
	    return (result);
	}
	result += ch;
    }
}

/*
 * relay_reset -- Reset the relay board.
 *
 * All relays are off
 * All GPIO set as input
 */
void relay_reset(void)
{
    raw_relay("reset");// Get the version of the relay board

    // Sets all the GPIO pins into the read state
    for (int i = 0; i < 10; ++i) 
	gpio_status(i);
}

/********************************************************
 * find_device -- Locate the relay device		*
 *							*
 * Returns						*
 * 	Name of the device				*
 ********************************************************/
static const char* find_device(void)
{
    if (access(RELAY_DEVICE1, R_OK) == 0) return (RELAY_DEVICE1);
    if (access(RELAY_DEVICE2, R_OK) == 0) return (RELAY_DEVICE2);
    if (access(RELAY_DEVICE3, R_OK) == 0) return (RELAY_DEVICE3);
    throw(relay_error("Could not find device"));
}

/*
 * Open the relay device 
 */
void relay_setup(void)
{
    if (simulate)
	return;

    struct termios tio;			// Terminal settings

    // Set raw mode
    memset(&tio,0,sizeof(tio));

    tio.c_iflag=0;
    tio.c_oflag=0;

    tio.c_cflag=CS8|CREAD|CLOCAL;       // 8 bits, no parity, 

    tio.c_lflag=0;

    tio.c_cc[VMIN]=1;			// Wait for at least one character
    tio.c_cc[VTIME]=5;			// Allow short time between characters

    const char* device = find_device();

    relay_fd = open(device, O_RDWR);      
    if (relay_fd < 0) 
	throw(relay_error("Could not open device "));

    cfsetospeed(&tio,B115200);            // 115200 baud
    cfsetispeed(&tio,B115200);            // 115200 baud

    if (tcsetattr(relay_fd, TCSANOW, &tio) != 0)
	throw(relay_error("Could not set speed"));

    // String to start the relay running
    static const char init_string[] = "\r\r\r";
    if (write(relay_fd, init_string, sizeof(init_string)-1) != sizeof(init_string)-1)
	throw(relay_error("Init string write error"));

    while (1) {
	// The inforation for the poll
	struct pollfd poll_in[] = {
		{ relay_fd, POLLIN, 0}
	};
	struct timespec timeout = {1, 500000000};	// Timeout is 1/2 second
	if (ppoll(poll_in, 1, &timeout, NULL) <= 0)
	    break;

	char ch;	// Character from the device
	if (read(relay_fd, &ch, 1) != 1) 
	    throw(relay_error("Read error -- initial sync"));
    }

    std::string ver = raw_relay_response("ver");// Get the version of the relay board
    if ((ver != "00000001") && (ver != "00000008"))
	throw(relay_error("Could not get version"));
}

/*
 * relay_status -- Display the status of a given relay
 */
std::string relay_status(const enum RELAY_NAME relay_number)
{
    if (simulate)
	return("0");
    // The command we are using
    std::ostringstream cmd;

    cmd << "relay read " << std::hex << std::uppercase << relay_number << std::dec;

    // Send comand, get result
    std::string result = raw_relay_response(cmd.str());
    return (result);
}
/*
 * gpio_status -- Display the status of a given gpio
 *
 * Parameters
 * 	gpio_number -- Number of the GPIO to read
 */
std::string gpio_status(const int gpio_number)
{
    if (simulate)
	return("1");
    // The command we are using
    std::ostringstream cmd;

    cmd << "gpio read " << gpio_number;

    // Send comand, get result
    std::string result = raw_relay_response(cmd.str());
    return (result);
}
/*
 * relay -- Set the state of a relay
 *
 * Parameters
 * 	thread_name -- Name of who's turning on the relay
 * 	relay_name -- The name of the relay
 * 	state -- The state we want to set the realy to
 */
void relay(
	const char* const thread_name,	// Name of the thread doing the change
	const enum RELAY_NAME relay_name, // The name of the relay
	const enum RELAY_STATE state	// The state of the relay
) {
    if (verbose) {
	syslog(LOG_INFO, "THREAD: %s RELAY %d: STATE: %s",
	    thread_name, static_cast<int>(relay_name),
	    (state == RELAY_STATE::RELAY_ON ? "On" : "Off"));
    }
    if (simulate)
	return;
    std::ostringstream cmd;
    cmd << "relay " << 
	(state == RELAY_STATE::RELAY_ON ? "on" : "off") << ' ' <<
	std::hex << std::uppercase << static_cast<int>(relay_name) << std::dec;
    raw_relay(cmd.str());
}

