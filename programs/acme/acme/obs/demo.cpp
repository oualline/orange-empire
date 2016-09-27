/*
 * Wait for the button, then demo the signal
 *
 * Usage:
 * 	demo [-v] [-s]
 *
 * 	-v Verbose 
 * 	-s Simulate
 */
#include <iostream>
#include <fstream>

#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <poll.h>

#include <wiringPi.h>

#include "relay.h"
#include "buttons.h"
#include "hw.h"

// ### fixme -- add options to turn on and things
bool verbose = false;		// Chatter
bool simulate = false;		// Do not do the work

static const int PASSWORD_TIMEOUT = (30 * 1000);	// 30 seconds to enter the password

/********************************************************
 * byebye -- Respond to a termination and stop
 ********************************************************/
static void byebye(int)
{
    exit(0);
}

/********************************************************
 * setup the GPIO stuff				
 ********************************************************/
static void setup_gpio(void)
{
    if (wiringPiSetup() != 0) {
	std::cerr << "ERROR: wiringPiSetup failed" << std::endl;
	exit(8);
    }
    pinMode(GPIO_START_BUTTON, INPUT);	// Start button is an input

    // Add in the pull up resistor
    pullUpDnControl(GPIO_START_BUTTON, PUD_UP);
}

/********************************************************
 * main_loop -- process everything for the outer loop
 ********************************************************/
static void main_loop()
{
    while (true)
    {
	std::cout << "Button loop\r" << std::endl;
	waitForInterrupt(GPIO_START_BUTTON, -1);

	while (true)
	{
	    if (digitalRead(GPIO_START_BUTTON) != 0)
	    {
		tv_on();
		// ### Get from config
		system("./long-demo");
		tv_off();
	    }
	}
    }
}

static void usage(void)
{
    std::cout << "Usage demo [-s] [-v] " << std::endl;
    exit(8);
}

int main(int argc, char* argv[])
{
    while (true) {
	int opt = getopt(argc, argv, "vs");
	if (opt < 0)
	    break;
	
	switch (opt)
	{
	    case 'v':
		verbose = true;
		break;
	    case 's':
		simulate = true;
		break;
	    default:
		usage();
	}
    }
    if (optind < argc)
	usage();
    setup_gpio();
    signal(SIGTERM, byebye);
    signal(SIGINT, byebye);
    relay_setup();

    main_loop();
    // not reached
}
