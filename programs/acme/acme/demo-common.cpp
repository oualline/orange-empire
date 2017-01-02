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

bool verbose = false;		// Chatter
bool simulate = false;		// Do not do the work
static bool no_button = false;	// Do not wait for button

config acme_config;	// The configuraiton

static const ssize_t FB_SIZE = 530432;	// Size of the frame buffer

static int fb_fd;	// Frame buffer fd
static void* fb_ptr;	// The FB data

static pid_t last_pid = 0;
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
 * Stop saying things
 ********************************************************/
void stop_say(void)
{
    if (last_pid >= 0)
    {
	waitpid(last_pid, NULL, 0);
    }
    last_pid = -1;
}
/********************************************************
 * Display an image on the screen
 *
 * Parameters
 * 	image The image to display
 *********************************************************/
void image(const char* const image)
{
    stop_say();
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
 * Send sound to the speaker
 *
 * Parameters
 * 	words -- File containing the words
 *********************************************************/
void say(const char* const words)
{
    std::ostringstream cmd;	// Command to execute
    stop_say();
    cmd << "mpg321 -q acme.sound/" << words << " 2>&1 >/dev/null";
    if (verbose)
	std::cout << "Say " << cmd.str() << std::endl;
    last_pid = fork();	// Fork, get pid

    if (last_pid == 0)
    {
	// Could do an exec here
	system(cmd.str().c_str());
	_exit(0);
    }
}
    

/********************************************************
 * Tell user how to use us
 ********************************************************/
static void usage(void)
{
    std::cout << "Usage is " << DEMO_NAME << " [-s] [-v] [-r] [-n]" << std::endl;
    std::cout << "	-r Simulate " << std::endl;
    std::cout << "	-v Verbose " << std::endl;
    std::cout << "	-s syslog -> stdout " << std::endl;
    std::cout << " 	-n Start demo (and loop demo) with no button press" << std::endl;
    exit(8);
}

/********************************************************
 * main_loop -- process everything for the outer loop
 ********************************************************/
static void main_loop()
{
    image("idle.fb");
    while (true)
    {
	if (! no_button)
	    waitForInterrupt(GPIO_START_BUTTON, -1);

	bool button  = (digitalRead(GPIO_START_BUTTON) != 0);
	if (button || (no_button))
	{
	    //##tv_on();
	    do_demo();
	    //##tv_off();
	    relay_reset();	// Clear everything just in case
	    image("idle.fb");
	}
    }
}
/********************************************************
 * setup the GPIO stuff				
 ********************************************************/
static void setup_gpio(void)
{
    if (wiringPiSetup() != 0) {
	std::cerr << "ERROR: wiringPiSetup failed" << std::endl;
	exit(8);
    }
    pinMode(GPIO_START_BUTTON, INPUT);	// Start button is an input

    // Add in the pull up resistor
    pullUpDnControl(GPIO_START_BUTTON, PUD_UP);
}

int main(int argc, char* argv[])
{
    bool stdout_log = false;	// Standard out to debug
    while (true) {
	int opt = getopt(argc, argv, "svrn");
	if (opt < 0)
	    break;
	switch (opt)
	{
	    case 'v':
		verbose = true;
		break;
	    case 's':
		stdout_log = true;
		break;
	    case 'r':
		simulate = true;
		break;
	    case 'n':
		no_button = true;
		break;
	    default:
		usage();
	}
    }
    // Open up the syslog system
    openlog(DEMO_NAME, stdout_log ? LOG_PERROR : 0, LOG_USER); 

    relay_setup();
    relay_reset();
    fb_fd = open("/dev/fb0", O_RDWR);	// Open the frame buffer
    if (fb_fd < 0)
	die("Could not open /dev/fb0");

    ssize_t length = (FB_SIZE + sysconf(_SC_PAGE_SIZE)-1) / sysconf(_SC_PAGE_SIZE);
    length *= sysconf(_SC_PAGE_SIZE);

    // Length is the size of the fame buffer
    fb_ptr = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_ptr == MAP_FAILED)
	die("Unable to mmap frame buffer");

    setup_gpio();
    main_loop();
    exit(0);
}
