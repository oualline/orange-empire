/*
 * Run the semaphore for rail giants
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
#include <sys/wait.h>

#include <wiringPi.h>

#include "relay.h"
static const int GPIO_ON = 5;		// ON/OFF toggle
static const int GPIO_NOW = 4;		// Execute now

bool verbose = false;		// Chatter
bool simulate = false;		// Do not do the work

/********************************************************
 * on_interrupt -- Field an interrupt on which we do	*
 * 	nothing.					*
 ********************************************************/
static void on_interrupt(void)
{
    // Do nothing
}

/********************************************************
 * die -- Output a message and die
 *
 * Parameters
 * 	msg The message telling us why we died
 ********************************************************/
void die(const char* const msg)
{
    std::cout << msg << std::endl;
    exit(8);
}
/********************************************************
 * byebye -- Respond to a termination and stop
 ********************************************************/
static void byebye(int)
{
    die("Terminated by signal");
}

/********************************************************
 * move_arms -- Move the arms				*
 *							*
 * Designed to be called from multiple threads and 	*
 * to ignore multiple calls.				*
 ********************************************************/
static void move_arms(void)
{
    static bool moving = false;	// Are we moving
    static pthread_mutex_t move_lock = PTHREAD_MUTEX_INITIALIZER;
    
    
    if (pthread_mutex_lock(&move_lock) != 0)
	die("Unable to obtain mutex");
    if (moving) {
	if (pthread_mutex_unlock(&move_lock) != 0)
	    die("Unable to release mutex");
	return;
    }
    moving = true;
    if (pthread_mutex_unlock(&move_lock) != 0)
	die("Unable to release mutex");
    
    relay("giant", UPPER_SEMAPHORE, RELAY_STATE::RELAY_ON);
    sleep(30);
    relay("giant", LOWER_SEMAPHORE, RELAY_STATE::RELAY_ON);
    sleep(30);
    relay("giant", UPPER_SEMAPHORE, RELAY_STATE::RELAY_OFF);
    sleep(30);
    relay("giant", LOWER_SEMAPHORE, RELAY_STATE::RELAY_ON);
    sleep(30);

    relay("giant", UPPER_SEMAPHORE, RELAY_STATE::RELAY_OFF);
    relay("giant", LOWER_SEMAPHORE, RELAY_STATE::RELAY_OFF);
    moving = false;
}


/********************************************************
 * debounce -- Read a pin with debouncing of 1/10 	*
 * 	seconds.					*
 *							*
 * Parameters						*
 * 	gpio_pin Pin to read				*
 * 							*
 * Returns						*
 * 	State of the pin				*
 * 	or -1 for timeout
 ********************************************************/
static int debounce(const int gpio_pin, time_t wait_time = -1)
{
    if (wait_time > 0) {
	// Get the result of waiting for the interrupt
	int result = waitForInterrupt(gpio_pin, wait_time*1000);
	if (result <= 0) 
	    return (-1);
    } else
	waitForInterrupt(gpio_pin, -1);

    while (true)
    {
	// Wait for 1/10 second with no change
	if (waitForInterrupt(gpio_pin, 100) == 0)
	    break;
    }

    return (digitalRead(gpio_pin));
}


/********************************************************
 * instant_thread -- Handle the instant button		*
 ********************************************************/
static void* instant_thread(void*)
{
    while (true)
    {
	while (debounce(GPIO_NOW) == 0)
	    continue;
	move_arms();
    }
}

static std::string the_time(void)
{
    char time_str[100];	// The time as a string

    time_t now = time(nullptr);

    strftime(time_str, sizeof(time_str), "%T: ", localtime(&now));
    return (std::string(time_str));
}

/********************************************************
 * usage -- Tell the user how to use us
 ********************************************************/
static void usage(void)
{
    std::cout << "Usage giant [-s] [-v] " << std::endl;
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

    if (verbose || simulate)
	openlog("giant", 0, LOG_USER|LOG_PERROR); 
    else
	openlog("giant", 0, LOG_USER); 

    signal(SIGTERM, byebye);
    signal(SIGINT, byebye);

    relay_setup();
    relay("giant", UPPER_SEMAPHORE, RELAY_STATE::RELAY_OFF);
    relay("giant", LOWER_SEMAPHORE, RELAY_STATE::RELAY_OFF);

    if (wiringPiSetup() != 0) {
	std::cerr << "ERROR: wiringPiSetup failed" << std::endl;
	exit(8);
    }
    pinMode(GPIO_ON, INPUT);	// Start button is an input
    pinMode(GPIO_NOW, INPUT);	// So is the active control

    // Add in the pull up resistor
    pullUpDnControl(GPIO_ON, PUD_UP);
    pullUpDnControl(GPIO_NOW, PUD_UP);

    if (wiringPiISR (GPIO_ON, INT_EDGE_FALLING, &on_interrupt) != 0) {
	std::cerr << "ERROR: wiringPiISR failed" << std::endl;
	exit(8);
    }
    if (wiringPiISR (GPIO_NOW, INT_EDGE_BOTH, &on_interrupt)  != 0) {
	std::cerr << "ERROR: wiringPiISR failed" << std::endl;
	exit(8);
    }

    pthread_t id;	// ID of the instant thread
    pthread_create(&id, NULL, instant_thread, NULL);

    // Thread that handles the daily on/off
    while (true)
    {
	if (verbose) std::cout << the_time() << "Wait for on" << std::endl;
	// Wait until the system goes on
	if (debounce(GPIO_ON) == 0)
	    continue;

	if (verbose) std::cout << "Start periodic" << std::endl;
	// Cycle 40 times (10 hours * 4 times / hour)
	// Or until the pin goes off
	for (int count = 0; count < (4*10); ++count)
	{
	    if (verbose) std::cout << the_time() << "Move periodic" << std::endl;
	    time_t end_time = time(NULL) + 15 * 60; // Sleep for 14 minutes
	    move_arms(); 


	    while (time(NULL) < end_time)
	    {
		if (verbose) std::cout << "Sleep periodic " << end_time - time(NULL) << std::endl;
		int status = debounce(GPIO_ON, end_time - time(NULL));
		if (status < 0)
		    continue;
		break;
	    }
	    if (digitalRead(GPIO_ON) == 0) {
		if (verbose) std::cout << "Periodic off" << std::endl;
		break;
	    }
        }
    }
    // not reached
}
