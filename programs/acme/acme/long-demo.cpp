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
    ding_and_flash_both();
    h1.go(head::ARMS_AND_LIGHTS);
    h2.stop(head::ARMS_AND_LIGHTS);

    sleep(5);
    /*
     *0:15
     *	H2: Bell,Yellow -> Stop/Arms/Light
     */
    ding_and_flash_both();
    h1.stop(head::ARMS_AND_LIGHTS);
    sleep(3);
    /*
     *0:23	
     *	H1: Go/Arms/Light
     */
    ding_and_flash_both();
    h2.go(head::ARMS_AND_LIGHTS);

    /*0:25
     *    Display: Road Runner Acme
     *	Before Chuck Jones and the roadrunner got into the act
     *	Acme was a well respected name.
     */
    image("acme_rocket.fb");
    say("before_chuck_jones.mp3");
    sleep(2);
    /*
     *    Display Acme real
     *	It meant the TOP, Pinical, or best.  
     */
    image("acme_real.fb");
    say("acme_real.mp3");
    sleep(2);
    /* 
     *    Display Yp cover
     *	It also was popular because the name was listed
     *	at the beginning of the Yellow Pages (this was
     *	back before google.)
     */
    image("yp.fb");
    say("yp.mp3");
    /*
     *0:45	
     *	H1: Bell,Yellow -> Stop/Arms/Light
     */
    ding_and_flash_both();
    h1.stop(head::ARMS_AND_LIGHTS);
    sleep(3);
    /*0:50
     *	H2: Go/Arms/Light
     */
    h2.go(head::ARMS_AND_LIGHTS);
    /*
     *	Display: Acme light
     *
     *	The first traffic signal was made by John Peake Knight, a British railroad signal engineer.
     *	In the early 1920's states experemented with a variety of traffic control devices.
     */
    image("first.fb");
    say("the_first.mp3");
    stop_say();
    sleep(3);
    /*
     *	Display: Umberally
     *
     *	Some that had to be changed manually.
     */
    image("umbrellalight.fb");
    say("some_that.mp3");
    stop_say();
    /*
     *	Display: Stop / go / top
     *
     *	Some stuck the policeman in a tower.
     */
    image("go_stop_top.fb");
    say("stuck_top.mp3");
    stop_say();
    sleep(2);
    /*
     *	Display: Twist
     */
    image("twist.fb");
    //??say("twist.mp3");
    stop_say();
    sleep(2);
    /*	
     *	Display: Clock
     *	
     *	Some that looked like a giant clock, with pointers telling you where to go.
     */
    image("clock-signal.fb");
    say("clock.mp3");
    stop_say();
    sleep(2);
    /*
     *	Display: Acme Light
     *
     *	Los Angeles use a design created by the Acme Traffic Signal Company.
     */
    image("acme3.fb");
    say("la_acme.mp3");
    stop_say();
    sleep(3);
    /*
     *	Display: Tri-color
     *
     *	Later, the Automobile Club decided that so many different types of
     *	signals were not a good idea and came up with the three light sign
     *	we use now.
     */
    image("Traffic_Light_Tree_2014.fb");
    say("3color.mp3");
    stop_say();
    sleep(2);
    /*
     *0:60	Display: Acme day
     *
     *	During the day the Acme signal uses the arms only.   
     *
     *	Lights used electricity and electricity cost money.
     */
    image("acme_day.fb");
    say("acme_day.mp3");
    stop_say();
     /*
     *0:65    Display: Acme night
     *
     *	But arms can not be seen at night so the lights were
     *	used at night.
     */
    image("acme_night.fb");
    say("acme_night.mp3");
    stop_say();

    /*
     *0:70:	Display: Acme Yellow
     *
     *	Very late at night, when there was little traffic, the 
     *	signal would blink yellow.
     */
    image("acme_yellow.fb");
    say("acme_yellow.mp3");
    stop_say();
    /*
     *0:75:	The signal will now cycle through, day, evening, night mode,
     *	and light night operations.
     */
    say("cycle.mp3");
    sleep(5);
    image("day.fb");

    // Day
    h1.lights_off();
    h2.lights_off();
    for (int i = 0; i < 2; ++i)
    {
	ding_both();
	h2.stop(head::ARMS_ONLY);
	sleep(2);
	h1.go(head::ARMS_ONLY);
	sleep(5);

	ding_both();
	h1.stop(head::ARMS_ONLY);
	sleep(2);
	h2.go(head::ARMS_ONLY);
	sleep(5);
    }
    h1.stop(head::LIGHTS_ONLY);
    h2.go(head::LIGHTS_ONLY);

    image("evening.fb");
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
    // Night
    h1.fold_arms();
    h2.fold_arms();
    image("night.fb");
    for (int i = 0; i < 2; ++i)
    {
	ding_both();
	h2.stop(head::LIGHTS_ONLY);
	sleep(2);
	h1.go(head::LIGHTS_ONLY);
	sleep(5);

	ding_both();
	h1.stop(head::LIGHTS_ONLY);
	sleep(2);
	h2.go(head::LIGHTS_ONLY);
	sleep(5);
    }
    h1.lights_off();
    h2.lights_off();
    image("late.fb");

    // Flashing late at night
    for (int i = 0; i < 10; ++i)
    {
	flash_both();
	sleep(1);
    }
#if 0
#define BEAT 2
    // Shave an a haircut
    h1.bell();
    sleep_10(BEAT);

    h1.bell();
    sleep_10(BEAT/2);

    h1.bell();
    sleep_10(BEAT/2);

    h1.bell();
    sleep_10(BEAT);

    h1.bell();
    sleep_10(BEAT*2);

    h2.bell();
    sleep_10(BEAT);

    h2.bell();
    sleep_10(BEAT);
#endif
}

