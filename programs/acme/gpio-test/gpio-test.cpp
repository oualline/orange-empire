/*
 * Test the GPIO for the "Start" button and the "Active" button.
 *
 * Run the program and see what comes out.
 */
#include <iostream>
#include <fstream>

#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <wiringPi.h>

static const int GPIO_START_BUTTON = 6;	// This is used to start the button
static const int GPIO_ACTIVE_PIN = 26;	// This tells us we are active


static void start_interrupt(void)
{
    std::cout << "Start interrupt " << digitalRead(GPIO_START_BUTTON) << std::endl;
}
static void active_interrupt(void)
{
    std::cout << "Active interrupt " << digitalRead(GPIO_ACTIVE_PIN) << std::endl;
}
int main()
{
    if (wiringPiSetup() != 0) {
	std::cerr << "ERROR: wiringPiSetup failed" << std::endl;
	exit(8);
    }
    pinMode(GPIO_START_BUTTON, INPUT);	// Start button is an input
    pinMode(GPIO_ACTIVE_PIN, INPUT);	// So is the active control

    // Add in the pull up resistor
    pullUpDnControl(GPIO_START_BUTTON, PUD_UP);
    pullUpDnControl(GPIO_ACTIVE_PIN, PUD_UP);

    if (wiringPiISR (GPIO_START_BUTTON, INT_EDGE_FALLING, &start_interrupt) != 0) {
	std::cerr << "ERROR: wiringPiISR failed" << std::endl;
	exit(8);
    }
    if (wiringPiISR (GPIO_ACTIVE_PIN, INT_EDGE_BOTH, &active_interrupt)  != 0) {
	std::cerr << "ERROR: wiringPiISR failed" << std::endl;
	exit(8);
    }

    while(1)
    {
	std::cout << "Start " << digitalRead(GPIO_START_BUTTON) << std::endl;
	sleep(1);
    }
}
