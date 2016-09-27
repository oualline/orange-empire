/*
 * Power -- Automatic power control for the signal garden
 *
 * Turns on and off the garden according to a time.
 * Will override the timing based on the state of the override switch.
 */
static const char* const BUTTON_PROG = "/home/garden/bin/button_avr";
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <syslog.h>
#include <signal.h>
#include <unistd.h>

#include <wiringPi.h>

bool stdout_log = false;	// Send log messages to stdout

// Garden pid file
#define GARDEN_PID  "/var/run/garden.pid"
#define BUTTON_PID  "/var/run/button.pid"

static const int START_HOUR = 8;	// Turn on at 8am
static const int STOP_HOUR = 19;	// Stop at 7pm

static const int OVER_POWER_ON = 24;	// Pin to read the power on override
static const int OVER_POWER_OFF = 22;	// Pin to read the power off override

static const int POWER_CONTROL = 23;	// Power control bit


// Current state of the power switch
static enum POWER_STATE {POWER_UNKNOWN, POWER_ON, POWER_OFF} power_state = POWER_UNKNOWN;

/*
 * kill_garden -- Kill the garden and button programs
 */
static void kill_garden(void)
{
    {
	// Get the garden pid
	std::ifstream pid_file(GARDEN_PID);
	if (!pid_file.bad()) {
	    pid_t pid;	// Pid to kill
	    pid_file >> pid;
	    if (pid != 0) {
		syslog(LOG_INFO, "Killing %d\n", pid);
		kill(pid, SIGTERM);
	    }
	}
    }
    // Get the button pid
    std::ifstream pid_file(BUTTON_PID);
    if (!pid_file.bad()) {
	pid_t pid;	// Pid to kill
	pid_file >> pid;
        if (pid != 0) {
	    syslog(LOG_INFO, "Killing %d\n", pid);
	    kill(pid, SIGTERM);
	}
    }
}
/*
 * start_garden -- Start the garden program
 */
static void start_garden(void) {
    system("chmod a+rw /dev/ttyACM0");
    system("chmod a+rw /dev/input/by-id/usb-PoLabs*");

    pid_t pid = fork();
    if (pid != 0)
	return;
    openlog("power", stdout_log ? LOG_PERROR : 0, LOG_USER); 
    syslog(LOG_INFO, "Start garden (pid=%d)", pid);
#if 1
    execl("/home/garden/bin/keep_running",
	    "keep_running",
	    "-p" GARDEN_PID,
	    "/home/garden/bin/garden",
	    NULL);
#else
    execl("/home/garden/bin/garden",
	  "-s", "-d", "-v",
	    NULL);
#endif
    return;
}
/*
 * start_button -- Start the button program
 */
static void start_button(void) {

    pid_t pid = fork();
    if (pid != 0)
	return;

    openlog("power", stdout_log ? LOG_PERROR : 0, LOG_USER); 
    syslog(LOG_INFO, "Start button (pid=%d)", pid);
    execl("/home/garden/bin/keep_running",
	    "keep_running",
	    "-p" BUTTON_PID,
	    BUTTON_PROG,
	    NULL);
}
int main(int argc, char* argv[])
{
    if (geteuid() != 0) {
	std::cout << "I need root" << std::endl;
	exit(8);
    }
    //	-- s log to stdout
    int opt;	// Option we are looking
    while ((opt = getopt(argc, argv, "s")) != -1) {
	switch (opt) {
	    case 's':
		stdout_log = true;
		break;
	    default: /* '?' */
		std::cerr << "Unknown option " << std::endl;
		exit(8);
	}
    }
    // Open up the syslog system
    openlog("power", stdout_log ? LOG_PERROR : 0, LOG_USER); 
    syslog(LOG_INFO, "Starting power program");

    wiringPiSetupPhys();	// Setup the GPI
    pinMode(OVER_POWER_ON, INPUT);
    pullUpDnControl(OVER_POWER_ON, PUD_OFF);

    pinMode(OVER_POWER_OFF, INPUT);
    pullUpDnControl(OVER_POWER_OFF, PUD_OFF);

    pinMode(POWER_CONTROL, OUTPUT);

    while (1) {
	enum POWER_STATE new_state = POWER_UNKNOWN;	// State we want to be in
	std::string modifier = "none";

	if (digitalRead(OVER_POWER_ON) == 0) {
	    new_state = POWER_ON;
	    modifier = "[forced]";
	}
	if (digitalRead(OVER_POWER_OFF) == 0) {
	    new_state = POWER_OFF;
	    modifier = "[forced]";
	}
	
	if (new_state == POWER_UNKNOWN) {
	    time_t now;	// The current time
	    time(&now);	// Get the time

	    struct tm now_tm; // Now as a TM time

	    if (localtime_r(&now, &now_tm) == NULL) {
		std::cout << "ERROR: Can't tell time " << std::endl;
		exit(8);
	    }

	    if ((now_tm.tm_hour < START_HOUR) || (now_tm.tm_hour >= STOP_HOUR)) {
		new_state = POWER_OFF;
	    } else {
		new_state = POWER_ON;
	    }
	    modifier = "[scheduled]";
	}
	if (new_state != power_state) {
	    // The message for the log
	    std::string log_msg = "Changing state to ";

	    switch (new_state) {
		case POWER_ON:
		    digitalWrite(POWER_CONTROL, 1);
		    log_msg += "on";
		    break;
		case POWER_OFF:
		    kill_garden();
		    digitalWrite(POWER_CONTROL, 0);
		    log_msg += "off";
		    break;
		default:
		    std::cout << "Internal error " << std::endl;
		    exit(8);
	    }
	    log_msg += modifier;
	    syslog(LOG_INFO, log_msg.c_str());
	    power_state = new_state;

	    sleep(20);
	    if (power_state == POWER_ON) {
		start_garden();
		start_button();
		sleep(10);
	    }
	    syslog(LOG_INFO, "Power sleep done");
	}
	sleep(1);
    }
}
