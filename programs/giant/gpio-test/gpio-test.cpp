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

static const int GPIO_ON = 5;		// ON/OFF toggle
static const int GPIO_NOW = 4;		// Execute now


static void on_interrupt(void)
{
    static int old_on = -1;		// Old value of the pin
    int value = digitalRead(GPIO_ON);	// Get value of the on pin

    if (value == old_on)
	return;

    std::cout << "On interrupt " << value << std::endl;
    old_on = value;
}
static void now_interrupt(void)
{
    static int old_now = -1;	// Old value of this pin
    int value = digitalRead(GPIO_NOW);	// Value

    if (old_now == value)
	return;

    std::cout << "Now interrupt " << value << std::endl;
    old_now = value;
}
int main()
{
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
    if (wiringPiISR (GPIO_NOW, INT_EDGE_BOTH, &now_interrupt)  != 0) {
	std::cerr << "ERROR: wiringPiISR failed" << std::endl;
	exit(8);
    }

    while(1)
    {
	//std::cout << "++On  " << digitalRead(GPIO_ON) << std::endl;
	//std::cout << "++Now " << digitalRead(GPIO_NOW) << std::endl;
	sleep(1);
    }
}
