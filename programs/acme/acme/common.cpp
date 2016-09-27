#include <iostream>

#include <syslog.h>

#include "common.h"

/********************************************************
 * sleep for the given sec/10 time
 *
 * Parameter
 * 	sleep_time Time to sleep in 1/10 second
 *********************************************************/
void sleep_10(unsigned int sleep_time)
{
    struct timespec to_sleep;
    to_sleep.tv_sec = sleep_time / 10;
    to_sleep.tv_nsec = sleep_time % 10 * 100000000;

    struct timespec remain;

    while (true)
    {
	if (nanosleep(&to_sleep, &remain) == 0)
	    return;
	to_sleep = remain;
    }
}
/********************************************************
 * Perform a system command verbosely
 *
 * Parameters
 * 	command Command to execute
 *********************************************************/
void do_system(const char* const command)
{
    if (verbose)
	std::cout << "do_system(" << command << ")\r" << std::endl;
    syslog(LOG_DEBUG, "do_system(%s)", command);
    if (simulate)
	return;
    system(command);
}
/********************************************************
 * Turn the screen off 
 ********************************************************/
void tv_off(void)
{
    do_system("tvservice -o >/dev/null");
}
/********************************************************
 * Turn the screen on
 ********************************************************/
void tv_on(void)
{
    do_system("tvservice -p >/dev/null");
    do_system("chvt 2");
    do_system("chvt 1");
}
