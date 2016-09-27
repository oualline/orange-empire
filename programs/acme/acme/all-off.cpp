/*
 * Turn off all the acme signal relays
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
#include <sys/wait.h>

#include "relay.h"
#include "buttons.h"
#include "conf.h"
#include "hw.h"

bool verbose = false;		// Chatter
bool simulate = false;		// Do not do the work
int main()
{
    relay_setup();
    relay_reset();
    return (0);
}
