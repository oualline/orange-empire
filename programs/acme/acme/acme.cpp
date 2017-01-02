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

#include "relay.h"
#include "buttons.h"
#include "conf.h"
#include "hw.h"

bool verbose = false;		// Chatter
bool simulate = false;		// Do not do the work

static const int PASSWORD_TIMEOUT = (30 * 1000);	// 30 seconds to enter the password

struct termios oldInfo;  // Old mode settings
config acme_config;	// The configuraiton

/********************************************************
 * die -- Output a message and die
 *
 * Parameters
 * 	msg The message telling us why we died
 ********************************************************/
void die(const char* const msg)
{
    if (tcsetattr(STDIN_FILENO, 0, &oldInfo) != 0)
    {   
	std::cerr << "ERROR: Could not set mode for the keyboard" << std::endl;
	exit(8);
    }   
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

static const int NO_TIMEOUT = -1;	// Do not timeout on read
static const char TIMEOUT_CHAR = 0xFF;	// What we return in case of a timeout
/********************************************************
 * get_char -- Read a character
 *
 * Parameters 
 * 	timeout -- Timeout in milliseconds (NO_TIMEOUT for no timeout)
 *
 * Returns
 * 	The character or -1 (0xff) for none
 *********************************************************/
static char get_char(const int timeout)
{
    // What we are polling for
    struct pollfd fds[] = {{
	STDIN_FILENO,
	POLLIN,
	0}};

    // The result of our poll
   int result = poll(fds, 1, timeout);
   if  (result == 0)
       return (TIMEOUT_CHAR);

   if (result < 0) {
       die("poll failed");
   }
   char ch;		// The character we are reading
   if (read(STDIN_FILENO, &ch, 1) != 1)
       die("read failed");
   return (ch);
}
/********************************************************
 * read_line -- Read a line from the input
 *
 * Parameters
 * 	echo -- If set, echo the input
 * 	timeout	-- The time in ms to timeout the reading (-1 no timeout)
 *
 * Returns
 * 	The line read
 *
 * Note: Does not handle backspace yet
 *********************************************************/
static std::string read_line(const bool echo, const int timeout)
{
    std::string line;	// The line we are reading
    while (true)
    {
	char ch = get_char(timeout);	// Character to read

	if (ch == TIMEOUT_CHAR)
	    return ("");

	if (echo)
	    write(STDOUT_FILENO, &ch, 1);

	if ((ch == '\r') || (ch == '\n'))
	    break;
	line += ch;
    }
    return (line);
}

/********************************************************
 * diag_help -- Tell the diag user what to do
 ********************************************************/
static void diag_help(void)
{
    std::cout << "Lower case -- Alpine (H1).  Upper case Broadway (H2)\r" << std::endl;
    std::cout << std::endl;
    std::cout << "R toggle red light\r" << std::endl;
    std::cout << "G toggle green light\r" << std::endl;
    std::cout << "Y toggle yellow light\r" << std::endl;
    std::cout << std::endl;
    std::cout << "D toggle direction relay (please turn off quickly)\r" << std::endl;
    std::cout << "P toggle arm power relay (please turn off quickly)\r" << std::endl;
    std::cout << std::endl;
    std::cout << "B toggle bell relay (very quickly toggle please)\r" << std::endl;
    std::cout << std::endl;
    std::cout << "S Print the status of all relays\r" << std::endl;
    std::cout << std::endl;
    std::cout << "Q turn off all relays\r" << std::endl;
    std::cout << "=====================\r" << std::endl;
    std::cout << std::endl;
    std::cout << "H Print out this help text\r" << std::endl;
    std::cout << "X exit the diagnostic\r" << std::endl;
    std::cout << std::endl;
}

/********************************************************
 * toggle -- Toggle a relay
 *
 * Parameters
 * 	on Array indicating if this relay is on (updated)
 * 	relay_name Relay to toggle
 ********************************************************/
static void toggle(bool on[], const enum RELAY_NAME relay_name)
{
    on[relay_name] = ! on[relay_name];
    if (on[relay_name])
	relay("diag", relay_name, RELAY_ON);
    else
	relay("diag", relay_name, RELAY_OFF);
}

/********************************************************
 * Report the status of a relay
 *
 * Parameters
 * 	On the indicator if the relay is on
 * 	name Name of the relay
 * 	h1 Index of the h1 relay
 * 	h2 Index of the h2 relay
 *********************************************************/
static void status(const bool on[], const char* const name, const RELAY_NAME h1, const RELAY_NAME h2)
{
    std::cout << name << (on[h1] ? "On  " : "Off ") << (on[h2] ? "On  " : "Off ") << '\r' << std::endl;
}
/********************************************************
 * diagnostic -- Turn on and off things as needed
 ********************************************************/
static void diagnostic(void)
{
    relay_reset();	
    relay_reset();	// To make sure

    std::cout << "WARNING WARNING WARNING\r" << std::endl;
    std::cout << std::endl;
    std::cout << "This program is for test and diagnostic programs only.\r" << std::endl;
    std::cout << std::endl;
    std::cout << "**************************************\r" << std::endl;
    std::cout << "**** MISUSE CAN DAMAGE THE SIGNAL ****\r" << std::endl;
    std::cout << "**************************************\r" << std::endl;
    std::cout << std::endl;
    std::cout << "Please type \"Yes\" to indicate that you are willing\r" << std::endl;
    std::cout << "to take responsiblity for your actions.\r" << std::endl;

    // Get the confirmation string
    std::string confirm = read_line(true, NO_TIMEOUT);
    if (confirm != "Yes")
    {
	std::cout << "Not confirmed\r" << std::endl;
	return;
    }

    diag_help();

    bool on[LAST_RELAY+1] = {false};	// Do we have power to the relay

    while (true)
    {
	// Get a character to send to the relay
	char ch = get_char(NO_TIMEOUT);
	std::cout << ch << '\r' << std::endl;
	switch (ch)
	{
	    case 'r': toggle(on, H1_RED); break;
	    case 'R': toggle(on, H2_RED); break;
	    case 'g': toggle(on, H1_GREEN); break;
	    case 'G': toggle(on, H2_GREEN); break;
	    case 'y': toggle(on, H1_YELLOW); break;
	    case 'Y': toggle(on, H2_YELLOW); break;

	    case 'd': toggle(on, H1_MOTOR_DIR); break;
	    case 'D': toggle(on, H2_MOTOR_DIR); break;
	    case 'p': toggle(on, H1_MOTOR_POWER); break;
	    case 'P': toggle(on, H2_MOTOR_POWER); break;

	    case 'f': toggle(on, H1_FOLD); break;
	    case 'F': toggle(on, H2_FOLD); break;

	    case 'b': toggle(on, H1_BELL); break;
	    case 'B': toggle(on, H2_BELL); break;

	    case 's':
	    case 'S':
		  std::cout << "Relay H1  H2\r" << std::endl;
		  status(on, "Red    ", H1_RED, H2_RED);
		  status(on, "Green  ", H1_GREEN, H2_GREEN);
		  status(on, "Yellow ", H1_YELLOW, H2_YELLOW);
		  std::cout << std::endl;
		  status(on, "Dir.   ", H1_MOTOR_DIR, H2_MOTOR_DIR);
		  status(on, "Power  ", H1_MOTOR_POWER, H2_MOTOR_POWER);
		  std::cout << std::endl;
		  status(on, "Fold   ", H1_FOLD, H2_FOLD);
		  std::cout << std::endl;
		  status(on, "Bell   ", H1_BELL, H2_BELL);
		  break;

	    case 'Q':
	    case 'q':
		  relay_reset();
		  memset(on, false, sizeof(on));
		  break;
	    case 'H':
	    case 'h':
		  diag_help();
		  break;
	    case 'X':
	    case 'x':
		  relay_reset();
		  return;
	    default:
		  std::cout << "Unknown command " << ch << '\r' << std::endl;
	}
    }
}

/********************************************************
 * Starts the demo process and returns the pid of it
 *
 * Returns
 *	pid of the demo
 ********************************************************/
static pid_t start_demo_process()
{
    // Fork off the demo
    pid_t my_pid = fork();
    if (my_pid != 0)
	return (my_pid);

    // The arguements to the demo
    const char* args[3] = {NULL, NULL, NULL};
    int n_args = 0;
    if (verbose) {
	args[n_args] = "-v";
	++n_args;
    }
    if (simulate) {
	args[n_args] = "-s";
	++n_args;
    }

    std::string demo_name = acme_config.get_demo_name();
    switch (demo_name.at(0))
    {
	case 'l':
	    if (access("./long-demo", X_OK) == 0)
		execv("./long-demo", const_cast<char* const *>(args));
	    else
		execv("/home/garden/bin/long-demo", const_cast<char* const *>(args));
	    die("execv failed");
	case 's':
	    if (access("./short-demo", X_OK) == 0)
		execv("./short-demo", const_cast<char* const *>(args));
	    else
		execv("/home/garden/bin/short-demo", const_cast<char* const *>(args));
	    die("execv failed");
	case 'w':
	    if (access("./wind-demo", X_OK) == 0)
		execv("./wind-demo", const_cast<char* const *>(args));
	    else
		execv("/home/garden/bin/wind-demo", const_cast<char* const *>(args));
	    die("execv failed");
	default:
	    die("Impossible demo type");
    }
}

/********************************************************
 * do_cmd -- Execute a single manual command 		*
 *							*
 * Returns						*
 * 	Command character (if not handled by this 	*
 * 	function)					*
 ********************************************************/
static char do_cmd(void)
{
    while (true)
    {
	tcflush(STDIN_FILENO, TCIFLUSH);	// Clear out previous commands
	char cmd = tolower(get_char(NO_TIMEOUT));

	switch (cmd)
	{
	    case 'x':
		return (true);
	    case '1':
		h1.fold_arms();
		acme_config.set_h1_arms(false);
		break;
	    case '2':
		acme_config.set_h1_arms(true);
		break;
	    case '9':
		h1.fold_arms();
		acme_config.set_h2_arms(false);
		break;
	    case '0':
		acme_config.set_h2_arms(true);
		break;
	    case 'o':
		ding_and_flash_both();	
		break;
	    case 'p':
		ding_and_flash_both();	
		usleep(500000);
		ding_and_flash_both();	
		usleep(500000);
		ding_and_flash_both();	
		usleep(500000);
		break;
	    default:
		return (false);
	}
    }
}

// The manual status
enum class MANUAL_STATE {A_GO_B_STOP, A_STOP_B_STOP, A_STOP_B_GO, A_STOP_B_STOP2};

// Given a state, go to the next state
static const MANUAL_STATE next_state[] = { 
    MANUAL_STATE::A_STOP_B_GO, 
    MANUAL_STATE::A_STOP_B_STOP2, 
    MANUAL_STATE::A_GO_B_STOP,
    MANUAL_STATE::A_STOP_B_STOP 
};

/********************************************************
 * Set the state of the signal
 ********************************************************/
static void set_state(const MANUAL_STATE state)
{
    // Quick help text
    std::cout << "x-exit 1/2:En/Dis Br. 9/0:En/Dis Al. a/Alpine b/broadway O-Ding P-DDD ";

    switch (state)
    {
	case MANUAL_STATE::A_GO_B_STOP:
	    std::cout << "Alpine GO, Broadway STOP\r" << std::endl;
	    if (h2.is_go())
		ding_and_flash_both();	
	    h2.stop(head::SIGNAL_HOW::AS_CONF, acme_config.get_h2_arms());
	    h1.go(head::SIGNAL_HOW::AS_CONF,   acme_config.get_h2_arms());
	ding_and_flash_both();
	    break;
	case MANUAL_STATE::A_STOP_B_STOP:
	    std::cout << "Alpine STOP, Broadway STOP\r" << std::endl;
	    if (h1.is_go() || h2.is_go())
		ding_and_flash_both();	
	    h1.stop(head::SIGNAL_HOW::AS_CONF, acme_config.get_h1_arms());
	    h2.stop(head::SIGNAL_HOW::AS_CONF, acme_config.get_h2_arms());
	    break;
	case MANUAL_STATE::A_STOP_B_GO:
	    std::cout << "Alpine STOP, Broadway GO\r" << std::endl;
	    if (h1.is_go())
		ding_and_flash_both();	
	    h1.stop(head::SIGNAL_HOW::AS_CONF, acme_config.get_h1_arms());
	    h2.go(head::SIGNAL_HOW::AS_CONF,   acme_config.get_h2_arms());
	    break;
	case MANUAL_STATE::A_STOP_B_STOP2:
	    std::cout << "Alpine STOP, Broadway GO\r" << std::endl;

	    if (h1.is_go() || h2.is_go())
		ding_and_flash_both();	
	    h1.stop(head::SIGNAL_HOW::AS_CONF, acme_config.get_h1_arms());
	    h2.stop(head::SIGNAL_HOW::AS_CONF, acme_config.get_h2_arms());
	    break;
	default:
	    abort();
    }
}
/********************************************************
 * Manually change the signals				*
 ********************************************************/
static void manual()
{
    // The state of the machine
    MANUAL_STATE cur_state = MANUAL_STATE::A_STOP_B_STOP;
    set_state(cur_state);

    bool done = false;	// Are we done yet
    while (!done)
    {
	switch (do_cmd())
	{
	    case 'a':
		cur_state = MANUAL_STATE::A_GO_B_STOP;
		break;
	    case 'b':
		cur_state = MANUAL_STATE::A_STOP_B_GO;
		break;
	    case 'x':
		done = true;
		break;
	}
	set_state(cur_state);
	cur_state = next_state[static_cast<unsigned int>(cur_state)];
    }
    h1.fold_arms();
    h2.fold_arms();
    relay_reset();
}
/********************************************************
 * set_demo -- Set the demo type			*
 ********************************************************/
static void set_demo(void)
{
    std::cout << "Set demo type" << std::endl;
    std::cout << "l -- Long demo\r" << std::endl;
    std::cout << "s -- Short demo\r" << std::endl;
    std::cout << "w -- Windy demo (lightly only)\r" << std::endl;
    std::cout << "Demo type: ";

    // Get the demo type character
    char ch = get_char(NO_TIMEOUT);
    switch (ch)
    {
	case 'l':
	    std::cout << "Setting long demo\r" << std::endl;
	    acme_config.set_demo_name("long");
	    break;
	case 's':
	    std::cout << "Setting short demo\r" << std::endl;
	    acme_config.set_demo_name("short");
	    break;
	case 'w':
	    std::cout << "Setting wind demo\r" << std::endl;
	    acme_config.set_demo_name("wind");
	    break;
	default:
	    std::cout << "Leaving demo setting unchanged\r" << std::endl;
	    break;
    }
}

/********************************************************
 * arm_time -- Set the arm time				*
 ********************************************************/
static void arm_time(void)
{
    std::cout << "Current arm time (in 1/10 seconds) " << acme_config.get_arm_time() << "\r" << std::endl;
    std::cout << "\r" << std::endl;
    std::cout << "WARNING WARNING WARNING\r" << std::endl;
    std::cout << std::endl;
    std::cout << "Setting the arm time to a bad value can wear out the motors on the sign\r" << std::endl;
    std::cout << std::endl;
    std::cout << "Please type \"Yes\" to indicate that you are willing\r" << std::endl;
    std::cout << "to take responsiblity for your actions.\r" << std::endl;

    // Get the confirmation string
    std::string confirm = read_line(true, NO_TIMEOUT);
    if (confirm != "Yes")
    {
	std::cout << "Not confirmed\r" << std::endl;
	return;
    }

    std::cout << "Arm time in 1/10 of a second: ";
    std::string arm_time_str = read_line(true, NO_TIMEOUT);
    for (auto ch: arm_time_str)
    {
	if (!isdigit(ch)) {
	    std::cout << "Illegal number -- time unchanged\r" << std::endl;
	    return;
	}
    }
    // The arm time as initeger
    uint32_t arm_time = atol(arm_time_str.c_str());
    if ((arm_time <= 0) || (arm_time > 200)) {
	std::cout << "Number exceeds limit -- time unchanged\r" << std::endl;
	return;
    }
    acme_config.set_arm_time(arm_time);
}
/********************************************************
 * volume -- Set the volume 
 ********************************************************/
static void volume(void)
{
    std::cout << "Volume (in %) [0-100]" << std::flush;

    std::string volume_percent = read_line(true, NO_TIMEOUT);
    for (auto ch: volume_percent)
    {
	if (!isdigit(ch)) {
	    std::cout << "Illegal number -- time unchanged\r" << std::endl;
	    return;
	}
    }
    // The arm time as initeger
    uint32_t volume_num = atol(volume_percent.c_str());
    if (volume_num > 100) {
	std::cout << "Number exceeds limit -- volume unchanged\r" << std::endl;
	return;
    }
    std::ostringstream cmd;	// Command to set sound level
    cmd << "amixer set PCM -- " << volume_num << "%" << std::endl;;

    // Execute the command
    do_system(cmd.str().c_str());

    // Write the configuraiton file
    std::ofstream vol_file("vol_cmd.sh");
    if (!vol_file.bad()) {
	vol_file << cmd.str();
	vol_file.close();
    }

}

/********************************************************
 * main_loop -- process everything for the outer loop	*
 ********************************************************/
static void main_loop()
{

    if (tcgetattr(STDIN_FILENO, &oldInfo) != 0)
    {   
	std::cerr << "ERROR: Could not set mode for the keyboard\r" << std::endl;
	exit(8);
    }   
    struct termios rawInfo;  // Raw mode setting
    rawInfo = oldInfo;
    cfmakeraw(&rawInfo);
    if (tcsetattr(STDIN_FILENO, 0, &rawInfo) != 0)
    {   
	std::cerr << "ERROR: Could not set mode for the keyboard\r" << std::endl;
	exit(8); 
    }   
    while (true)
    {
	//##tv_off();
	// Start the demo
	pid_t demo_pid = start_demo_process();

	char ch;	// A character we just read

	if (read(STDIN_FILENO, &ch, 1) != 1) {
	    //##tv_on();
	    std::cerr << "ERROR: Unable to read terminal " << errno << '\r' << std::endl;
	    exit(8);
	}
	//##tv_on();
	std::cout << "Waiting for demonstration to finish\r" << std::endl;

	if (kill(demo_pid, SIGTERM) != 0)
	    die("kill returned error");

	if (waitpid(demo_pid, NULL, 0) == 0)
	    die("waitpid returned error");

	std::cout << "Demonstration complete\r" << std::endl;

	system("clear");
	std::cout << "Password: " << std::flush;
	// The password or a timeout
	std::string password = read_line(false, PASSWORD_TIMEOUT);
	std::cout << "\r" << std::endl;
	if (password != acme_config.get_password()) {
	    std::cout << "Authentication error\r" << std::endl;
	    continue;
	}
	while (true) {
	    std::cout << "m -- manual mode\r" << std::endl;
	    std::cout << "d -- Configure demo type\r" << std::endl;
	    std::cout << "v -- Set volume\r" << std::endl;
	    std::cout << "x -- Return to tourist mode\r" << std::endl;
	    std::cout << "q -- Exit program\r" << std::endl;
	    std::cout << "\r" << std::endl;
	    std::cout << "1 -- Disable Alpine Arms\r" << std::endl;
	    std::cout << "2 -- Enable Alpine Arms\r" << std::endl;
	    std::cout << "9 -- Disable Broadway Arms\r" << std::endl;
	    std::cout << "0 -- Enable Broadway Arms\r" << std::endl;
	    std::cout << "\r" << std::endl;
	    std::cout << "D -- diagnostic(Expert)\r" << std::endl;
	    std::cout << "A -- configure arm motor time (expert)\r" << std::endl;
	    std::cout << "Command: ";

	    char ch = get_char(NO_TIMEOUT);
	    if ((ch == 'x') || (ch == 'X'))
		break;
	    if ((ch == 'q') || (ch == 'Q'))
		die("Bye bye");
	    switch (ch)
	    {
		case 'D':
		    std::cout << "Starting diagnostic\r" << std::endl;
		    diagnostic();
		    break;
		case 'A':
		    arm_time();
		    break;
		case 'v':
		    volume();
		    break;

		case 'd':
		    set_demo();
		    break;
		case 'm':
		    std::cout << "Starting manual mode\r" << std::endl;
		    manual();
		    break;
		case '1':
		    h1.fold_arms();
		    acme_config.set_h1_arms(false);
		    break;
		case '2':
		    acme_config.set_h1_arms(true);
		    break;
		case '9':
		    h1.fold_arms();
		    acme_config.set_h2_arms(false);
		    break;
		case '0':
		    acme_config.set_h2_arms(true);
		    break;
		default:
		    std::cout << "Can not understand command\r" << std::endl;
		    break;
	    }
	}
    }
}

/********************************************************
 * usage -- Tell the user how to use us
 ********************************************************/
static void usage(void)
{
    std::cout << "Usage acme [-s] [-v] " << std::endl;
    exit(8);
}

int main(int argc, char* argv[])
{
    openlog("acme", 0, LOG_USER); 
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
    signal(SIGTERM, byebye);
    signal(SIGINT, byebye);
    relay_setup();

    main_loop();
    // not reached
}
