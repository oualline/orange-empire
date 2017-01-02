/*
 * Run the ACME signals and 
 *
 * Run the program and and the signal works
 */
#include <iostream>
#include <fstream>
#include <sstream>

#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <poll.h>
#include <sys/wait.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <wiringPi.h>

#include "relay.h"
#include "hw.h"
#include "buttons.h"
#include "demo-common.h"

const char* const DEMO_NAME = "long-demo";	// Who we are

/**
 * Do a full demo
 */
void do_demo(void)
{
    /*
     *0:00
     *    Display: Title page		"The Acme Traffic Signal"
     *	H1: Bell,Yellow -> Stop/Arms/Light
     *	H2: Bell,Yellow -> Go/Arms/Light
     *     sleep 15
     *
     */
    image("slide1.fb");
    //say("wind.mp3");

    for (int i = 0; i < 2; ++i)
    {
	ding_and_flash_both();
	h2.stop(head::SIGNAL_HOW::LIGHTS_ONLY, false);
	sleep(2);
	h1.go(head::SIGNAL_HOW::LIGHTS_ONLY, false);
	sleep(5);

	ding_and_flash_both();
	h1.stop(head::SIGNAL_HOW::LIGHTS_ONLY, false);
	sleep(2);
	h2.go(head::SIGNAL_HOW::LIGHTS_ONLY, false);
	sleep(5);
    }
}
