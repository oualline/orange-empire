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

    
const char* const DEMO_NAME = "short-demo";
/**
 * Do a short demo
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
    //say("short.mp3");
    ding_and_flash_both();

    for (int i = 0; i < 2; ++i)
    {
	ding_both();
	h2.stop(head::ARMS_AND_LIGHTS);
	sleep(2);
	h1.go(head::ARMS_AND_LIGHTS);
	sleep(5);

	ding_both();
	h1.stop(head::ARMS_AND_LIGHTS);
	sleep(2);
	h2.go(head::ARMS_AND_LIGHTS);
	sleep(5);
    }
}

