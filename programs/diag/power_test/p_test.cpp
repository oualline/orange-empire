
/*
 * p-test -- Test power on/off
 */
#include <iostream>
#include <cstdlib>

#include <syslog.h>
#include <unistd.h>

#include <wiringPi.h>

static const int POWER_CONTROL = 23;	// Power control bit

int main()
{
    if (geteuid() != 0) {
	std::cout << "I need root" << std::endl;
	exit(8);
    }

    wiringPiSetupPhys();	// Setup the GPI

    pinMode(POWER_CONTROL, OUTPUT);

    while (1) {
	std::string dummy;	// Something to read

	std::cout << "On" << std::endl;
	digitalWrite(POWER_CONTROL, 1);

	std::getline(std::cin, dummy);

	std::cout << "Off" << std::endl;
	digitalWrite(POWER_CONTROL, 0);

	std::getline(std::cin, dummy);
    }
}
