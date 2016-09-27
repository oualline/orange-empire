/*
 * Run the ACME signals and 
 *
 * Run the program and and the signal works
 */
#include <iostream>
#include <sstream>

#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/fcntl.h>

#include <wiringPi.h>

#include "buttons.h"
#include "hw.h"
#include "conf.h"
#include "common.h"

bool verbose = false;
bool simulate = false;

config acme_config;	// The configuraiton

// ### make common
static int fb_fd;	// Frame buffer fd
static void* fb_ptr;	// The FB data
static const ssize_t FB_SIZE = 530432;	// Size of the frame buffer

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
 * setup the GPIO stuff				
 ********************************************************/
static void setup_gpio(void)
{
    if (wiringPiSetup() != 0) {
	std::cerr << "ERROR: wiringPiSetup failed\r" << std::endl;
	exit(8);
    }
    pinMode(GPIO_ACTIVE_PIN, INPUT);	// So is the active control

    // Add in the pull up resistor
    pullUpDnControl(GPIO_ACTIVE_PIN, PUD_UP);
}

/********************************************************
 * Starts the acme process and returns the pid of it
 *
 * Returns
 *	pid of the acme process
 ********************************************************/
static pid_t start_acme_process()
{
    if (verbose)
	std::cout << "Starting acme" << std::endl;

    if (simulate)
	return(-1);

    // Fork off the acme proces
    pid_t my_pid = fork();
    if (my_pid != 0)
	return (my_pid);

    // The arguements to the acme process
    const char* args[3] = {NULL, NULL, NULL};

    if (access("./acme", X_OK) == 0) {
	if (verbose)
	    std::cout << "start ./acme" << std::endl;
	if (!simulate)
	    execv("./acme", const_cast<char* const *>(args));
    }
    else {
	if (verbose)
	    std::cout << "start /home/garden/bin/acme" << std::endl;
	if (!simulate)
	    execv("/home/garden/bin/acme", const_cast<char* const *>(args));
    }
    return (-1);	
}
/********************************************************
 * Display an image on the screen
 *
 * Parameters
 * 	image The image to display
 *********************************************************/
void image(const char* const image)
{
    if (verbose)
	std::cout << "Image " << image << std::endl;
    std::ostringstream file;	// Command to execute
    file << "cooked/" << image;

    int in_fd = open(file.str().c_str(), O_RDONLY);
    if (in_fd < 0)
	die("Unable to open image file ");

    if (read(in_fd, fb_ptr, FB_SIZE) != FB_SIZE)
	die("Read of input file failed");

    int code = msync(fb_ptr, FB_SIZE, 0);
    if (code != 0)
	die("msync failure");

    close(in_fd);
}

/********************************************************
 * main_loop -- process everything for the outer loop
 ********************************************************/
static void main_loop()
{
    while (true)
    {
	/*
	 * Active button
	 * 	= 0 -- Off
	 * 	= 1 -- On
	 */
	waitForInterrupt(GPIO_ACTIVE_PIN, -1);

	// If we are not active, continue
	if (digitalRead(GPIO_ACTIVE_PIN) == 0)
	    continue;

	if (verbose)
	    std::cout << "Active turned on " << std::endl;

	sleep(5);
	// Debounce the switch
	// If not active, continue
	if (digitalRead(GPIO_ACTIVE_PIN) == 0)
	    continue;

	if (verbose)
	    std::cout << "Active turned on really" << std::endl;

	tv_on();
	image("idle.fb");
	
	// Start the acme program -- get the pid
	pid_t acme_pid = start_acme_process();

	while (true) {
	    waitForInterrupt(GPIO_ACTIVE_PIN, -1);

	    // See if this is a glitch
	    if (digitalRead(GPIO_ACTIVE_PIN) == 1)
		continue;

	    if (verbose)
		std::cout << "Active turned off\r" << std::endl;

	    sleep(5);
	    if (digitalRead(GPIO_ACTIVE_PIN) == 1)
		continue;
	    break;
	}
	if (verbose)
	    std::cout << "Active turned off really\r" << std::endl;

	if (acme_pid >= 0) {
	    kill(acme_pid, SIGINT);

	    if (waitpid(acme_pid, NULL, 0) == 0)
		die("waitpid returned error");
	}
	tv_off();
    }
}

/********************************************************
 * Tell user how to use this program
 ********************************************************/
static void usage()
{
    std::cout << "Usage:" << std::endl;
    std::cout << "    master [-v] [-s]" << std::endl;
    std::cout << "Options: " << std::endl;
    std::cout << "    -v -- verbose " << std::endl;
    std::cout << "    -s -- simulate " << std::endl;
    exit(8);
}
int main(int argc, char* argv[])
{
    openlog("master", 0, LOG_USER); 
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

    setup_gpio();

    fb_fd = open("/dev/fb0", O_RDWR);	// Open the frame buffer
    if (fb_fd < 0)
	die("Could not open /dev/fb0");

    ssize_t length = (FB_SIZE + sysconf(_SC_PAGE_SIZE)-1) / sysconf(_SC_PAGE_SIZE);
    length *= sysconf(_SC_PAGE_SIZE);

    // Length is the size of the fame buffer
    fb_ptr = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_ptr == MAP_FAILED)
	die("Unable to mmap frame buffer");
    
    tv_off();
    signal(SIGTERM, byebye);

    main_loop();
    // not reached
}
