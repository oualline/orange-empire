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

#include "relay.h"
 
bool simulate = false;
bool verbose = false;	// Chatter?
/*
 * status -- print the status of each relay an input
 */
static void status(void)
{
    for (int i = 0; i <= 16; ++i) {
	std::cout << "Relay " << i << " state " << 
		relay_status(static_cast<enum RELAY_NAME>(i)) << std::endl;
    }
    for (int i = 0; i < 2; ++i) {
	std::cout << "GPIO " << i << " state " << gpio_status(i) << std::endl;
    }
}

/*
 * cmd_relay -- Send an on/off command to a relay.
 *
 * Parameters
 * 	relay -- String with relay number
 * 	state -- state to change the relay to
 */
static void cmd_relay(const std::string& relay_name, enum RELAY_STATE state)
{
    relay("test", static_cast<enum RELAY_NAME>(atol(relay_name.c_str())), state);
}
/*
 * Display help text
 */
static void help(void)
{
    std::cout << "? -- Help" << std::endl;
    std::cout << "+<relay> -- Turn relay on " << std::endl;
    std::cout << "-<relay> -- Turn relay off " << std::endl;
    std::cout << "s -- Status " << std::endl;
    std::cout << "r -- Reset " << std::endl;
}
int main()
{
    relay_setup();

    while (1) {
	std::cout << "Cmd: " << std::flush;

	std::string cmd;	// Command from the user
	// Get the command
	std::getline(std::cin, cmd);
	if (cmd.length() <= 0) {
	    help();
	    continue;
	}
	switch (cmd.at(0)) {
	    case 's':
		status();
		break;
	    case '+':
		cmd_relay(cmd.substr(1), RELAY_STATE::RELAY_ON);
		break;
	    case '-':
		cmd_relay(cmd.substr(1), RELAY_STATE::RELAY_OFF);
		break;
	    case 'r':
		relay_reset();
		break;
	    default:
		help();
		break;
	}
    }
}
