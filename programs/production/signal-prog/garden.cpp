/*
 * Run the signal garden.
 *
 * Input comes in through:
 *    1) A socket connected to the input handler
 *    2) From an external socket (debug, maintenance)
 *    3) Console thread (debugging only)
 *
 * The system responds to the incoming events and runs the signals.
 *
 */
#include <iostream>
#include <map>
#include <sstream>

#include <cstdio>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <net/if.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <sys/poll.h>

#include "relay.h"
#include "device.h"

bool verbose = false;		// Chatter
bool simulate = false;		// Do not do the work
static bool debug = false;	// Do not go into background

/*------------------------------------------------------*/
/*------------------------------------------------------*/
// Configuration
/*------------------------------------------------------*/
/*------------------------------------------------------*/
// H2 Signal
static const int H2_WAIT = 5;	// Turn read for 5 seconds

// Track cars
static const int CAR_WAIT = 6;	

// 4 White lights
static const int W4_WAIT = 5;	// Wait five seconds between changes
// 3 Color lights
static const int C3_WAIT = 5;	// Wait five seconds between changes

// Wig Wag / Bell wait
static const int WW_WAIT = 15;	// Longer time for bells

// Noise reduction wait
static const int NOISE_WAIT = 7;	// 10 seconds each noise reduction step


typedef void *(*thread_function) (void *);

// List of all the handlers
enum HANDLER_ID {
    HANDLE_H2,		// H2 dwarf spotlight at entrance
    HANDLE_W4,		// 4 White light indicator (dwarf)
    HANDLE_C3,		// 3 color lights (set)
    HANDLE_CAR,		// Track car indicator
    HANDLE_LWW,		// Lower quadrant wig wag
    HANDLE_BELL,	// Crossing bell
    HANDLE_UWW,		// Upper quadrant wig wag
    HANDLE_NOISE,	// Noise supression function
    HANDLE_LAST		// Last handler (not a real item)
};

static const enum HANDLER_ID HANDLE_FIRST = HANDLE_H2;	// First handler

static enum HANDLER_ID button_handler_map[10] = {
    HANDLE_H2,		// [0] H2 dwarf spotlight at entrance
    HANDLE_W4,		// [1] 4 White light indicator (dwarf)
    HANDLE_C3,		// [2] 3 color lights (set)
    HANDLE_C3,		// [3] 3 color lights (set)
    HANDLE_CAR,		// [4] Track car indicator
    HANDLE_BELL,	// [5] Crossing bell (NC)
    HANDLE_LWW,		// [6] Lower quadrant wig wag
    HANDLE_UWW,		// [7] Upper quadrant wig wag
    HANDLE_H2,		// [8] H2 (NC)
    HANDLE_H2,		// [9] H2 (NC)
};

struct handler_info {
    thread_function funct;	// Function to handle the item
    sem_t sem;			// Semaphore that controls us
    const char* const name;	// Name of the handler
    enum HANDLER_ID id;		// ID Number of the handler
};

static const int SWITCH_NO_SOUND = 0;	// The number of the "No Sound" switch
static const int SWITCH_LOW_NOISE = 1;	// The number of the "Low sound" switch

static volatile bool low_noise_active = false;	// Are we in the low noise routine

// Are the signals under control of the low noise function
static enum {SIGNAL_NORMAL, SIGNAL_LOW_NOISE} signal_mode = SIGNAL_NORMAL;

/*
 * sem_clear -- Clear all pending semaphores
 *
 * Parameters
 * 	sem -- Semaphore to deal with
 */
static void sem_clear(sem_t* const sem)
{
    while (sem_trywait(sem) == 0) 
	continue;
}

/*
 * sem-wait-time -- Wait the specified amount of time or until a semaphore is triggered
 *
 * Parameters
 * 	sem -- Semaphore
 * 	wait -- How long to wait
 */
static void sem_wait_time(sem_t* const sem, const time_t wait)
{
    struct timespec wait_time;	// Time we are going wait until

    wait_time.tv_sec = time(NULL) + wait;
    wait_time.tv_nsec = 0;
    sem_timedwait(sem, &wait_time);
}
static void* handle_h2(void* me_v);
static void* handle_w4(void* me_v);
static void* handle_c3(void* me_v);
static void* handle_car(void* me_v);
static void* handle_lww(void* me_v);
static void* handle_uww(void* me_v);
static void* handle_bell(void* me_v);
static void* handle_noise(void* me_v);
/*
 * Array containing all the signal handlers 
 */
static struct handler_info handler_array[] = {
    {
	handle_h2,		// 0
	{{0}},
	"h2",
	HANDLE_H2
    }, {
	handle_w4,		// 1
	{{0}},
	"w4",
	HANDLE_W4
    }, {
	handle_c3,		// 2
	{{0}},
	"c3",
	HANDLE_C3
    }, {
	handle_car,		// 3
	{{0}},
	"car",
	HANDLE_CAR
    }, {
	handle_lww,		// 4
	{{0}},
	"lww",
	HANDLE_LWW
    }, {
	handle_bell,		// 5
	{{0}},
	"bell",
	HANDLE_BELL
    }, {
	handle_uww,		// 6
	{{0}},
	"uww",
	HANDLE_UWW
    }, {
	handle_noise,		// 7
	{{0}},
	"noise",
	HANDLE_NOISE
    }, {
	NULL,			// 8
	{{0}},
	"End Of List",
	HANDLE_LAST
    }
};

/*
 * push -- Push a button
 *
 * Returns
 * 	the result of the push
 */
static std::string push(enum HANDLER_ID id)
{
    if (sem_post(&handler_array[id].sem) == -1) {
	syslog(LOG_ERR, "ERROR: sem_post failed -- abort");
	exit(8);
    }
    return ("OK");
}
/*
 * handle_h2 -- Handle the h2 signal
 *
 * When the button is pressed, turn the signal red for a few seconds.
 *
 * Parameters
 * 	me_v -- Pointer to the information for this handler
 */
static void* handle_h2(void* me_v) {
    // Pointer to the information for the
    struct handler_info* me = reinterpret_cast<struct handler_info*>(me_v);
    
    while (true) {
	relay(me->name, H2_RELAY, RELAY_OFF);
	sem_clear(&me->sem);
	if (sem_wait(&me->sem) != 0) {
	    if (errno == EAGAIN)
		continue;
	    syslog(LOG_ERR, "h2: ERROR: Semaphore failed -- abort ");
	    exit(8);
	}
	relay(me->name, H2_RELAY, RELAY_ON);
	sem_wait_time(&me->sem, H2_WAIT);
    }
    return (NULL);
}


/*
 * generic_ww -- generic function to handle all wig wags and bell
 *
 * Parameters
 * 	me -- Information about me
 *	relay_id -- Relay to turn on and off
 */
static void generic_ww(struct handler_info* const me, const enum RELAY_NAME relay_id)
{
    while (true) {
	sem_clear(&me->sem);
	relay(me->name, relay_id, RELAY_OFF);

	if (sem_wait(&me->sem) != 0) {
	    if (errno == EAGAIN)
		continue;
	    syslog(LOG_ERR, "%s: ERROR: Semaphore failed -- abort ", me->name);
	    exit(8);
	}
	if (gpio_status(SWITCH_NO_SOUND) == "0") {
	    continue;
	}
	if (low_noise_active)
	    continue;

	if (gpio_status(SWITCH_LOW_NOISE) == "0") {
	    low_noise_active = true;
	    push(HANDLE_NOISE);
	}

	// Display track open
	relay(me->name, relay_id, RELAY_ON);
	sleep(WW_WAIT);
    }
}
/*
 * handle_lww -- Handle lower wig wag
 *
 * Signal rests on off.   Button press turns on
 *
 * Parameters
 * 	me_v -- Pointer to the information for this handler
 */
static void* handle_lww(void* me_v) 
{
    // Pointer to the information for the
    struct handler_info* me = reinterpret_cast<struct handler_info*>(me_v);

    generic_ww(me, LOWER_WW);
    return (NULL);
}
/*
 * handle_uww -- Handle upper wig wag
 *
 * Signal rests on off.   Button press turns on
 *
 * Parameters
 * 	me_v -- Pointer to the information for this handler
 */
static void* handle_uww(void* me_v) 
{
    // Pointer to the information for the
    struct handler_info* me = reinterpret_cast<struct handler_info*>(me_v);

    generic_ww(me, UPPER_WW);
    return (NULL);
}
/*
 * handle_bell -- Handle crossing bell
 *
 * Signal rests on off.   Button press turns on
 *
 * Parameters
 * 	me_v -- Pointer to the information for this handler
 */
static void* handle_bell(void* me_v) 
{
#if 0
    // Pointer to the information for the
    struct handler_info* me = reinterpret_cast<struct handler_info*>(me_v);

    generic_ww(me, BELL);
#endif
    return (NULL);
}

/*
 * handle_car -- Handle the track car indicator
 *
 * Signal rests on off.   Button press turns on
 *
 * Parameters
 * 	me_v -- Pointer to the information for this handler
 */
static void* handle_car(void* me_v) 
{
    // Pointer to the information for the
    struct handler_info* me = reinterpret_cast<struct handler_info*>(me_v);
    
    while (true) {
	sem_clear(&me->sem);

	if (signal_mode == SIGNAL_NORMAL) {
	    relay(me->name, TRACK_SEM_L, RELAY_OFF);
	    relay(me->name, TRACK_SEM_R, RELAY_OFF);
	    relay(me->name, TRACK_CAR, RELAY_OFF);
	}

	if (sem_wait(&me->sem) != 0) {
	    if (errno == EAGAIN)
		continue;
	    syslog(LOG_ERR, "car: ERROR: Semaphore failed -- abort ");
	    exit(8);
	}
	if (signal_mode == SIGNAL_LOW_NOISE) continue;

	// Train runs from right to left
	// First we have no train
	relay(me->name, TRACK_SEM_L, RELAY_ON);
	relay(me->name, TRACK_SEM_R, RELAY_ON);
	relay(me->name, TRACK_CAR, RELAY_ON);
	sem_wait_time(&me->sem, CAR_WAIT);
	if (signal_mode == SIGNAL_LOW_NOISE) continue;

	// Train has reached the track car indicator
	relay(me->name, TRACK_CAR, RELAY_OFF);
	sem_wait_time(&me->sem, CAR_WAIT);
	if (signal_mode == SIGNAL_LOW_NOISE) continue;

	// Train has reached the first semaphore
	relay(me->name, TRACK_CAR, RELAY_ON);
	relay(me->name, TRACK_SEM_L, RELAY_OFF);
	sem_wait_time(&me->sem, CAR_WAIT);
	if (signal_mode == SIGNAL_LOW_NOISE) continue;

	// Train has reached the second semaphore
	relay(me->name, TRACK_SEM_L, RELAY_ON);
	relay(me->name, TRACK_SEM_R, RELAY_OFF);
	sem_wait_time(&me->sem, CAR_WAIT);
	if (signal_mode == SIGNAL_LOW_NOISE) continue;

	// Now we turn things off because the demo is done
	relay(me->name, TRACK_CAR, RELAY_OFF);
	relay(me->name, TRACK_SEM_L, RELAY_OFF);

    }
    return (NULL);
}
/*
 * handle_noise -- Handle the noise reduction function
 *
 * Parameters
 * 	me_v -- Pointer to the information for this handler
 */
static void* handle_noise(void* me_v) 
{
    // Pointer to the information for the
    struct handler_info* me = reinterpret_cast<struct handler_info*>(me_v);
    
    while (true) {
	sem_clear(&me->sem);
	
	// Do not reset the signals while waiting 
	// They can in use by other functions.

	if (sem_wait(&me->sem) != 0) {
	    if (errno == EAGAIN)
		continue;
	    syslog(LOG_ERR, "car: ERROR: Semaphore failed -- abort ");
	    exit(8);
	}

	signal_mode = SIGNAL_LOW_NOISE;
	relay(me->name, TRACK_SEM_L, RELAY_ON);
	relay(me->name, TRACK_SEM_R, RELAY_ON);
	relay(me->name, TRACK_CAR, RELAY_ON);

	relay(me->name, C3_RED, RELAY_ON);
	relay(me->name, C3_YELLOW, RELAY_OFF);
	relay(me->name, C3_GREEN, RELAY_OFF);
	sleep(WW_WAIT);

	// Train runs from right to left (it just made the track car lights)
	relay(me->name, TRACK_CAR, RELAY_OFF);
	sleep(NOISE_WAIT);

	// Train is now as the left semaphore
	relay(me->name, TRACK_CAR, RELAY_ON);
	relay(me->name, TRACK_SEM_L, RELAY_OFF);
	sleep(NOISE_WAIT);

	// Car is at the right semaphore
	relay(me->name, TRACK_SEM_L, RELAY_ON);
	relay(me->name, TRACK_SEM_R, RELAY_OFF);
	sleep(NOISE_WAIT);

	// Car is clear of the car indicators, now just past the yellow light
	relay(me->name, TRACK_SEM_R, RELAY_ON);
	relay(me->name, C3_RED, RELAY_OFF);
	relay(me->name, C3_YELLOW, RELAY_ON);
	sleep(NOISE_WAIT);

	// We just cleared the last section of trake.  Green light
	relay(me->name, C3_YELLOW, RELAY_OFF);
	relay(me->name, C3_GREEN, RELAY_ON);

	// The track car indicators go back to demo mode
	relay(me->name, TRACK_CAR, RELAY_OFF);
	relay(me->name, TRACK_SEM_L, RELAY_OFF);
	relay(me->name, TRACK_SEM_R, RELAY_OFF);
	
	low_noise_active = false;
	sleep(NOISE_WAIT);
	relay(me->name, C3_GREEN, RELAY_OFF);
	signal_mode = SIGNAL_NORMAL;
    }
    return (NULL);
}
/*
 * handle_c3 -- Handle the three color light set
 *
 * Signal rests on "all off"
 * 	Button press moves to red -> yellow -> green -> all off
 * 	Button press during this sequence advances to the next state
 *
 * Parameters
 * 	me_v -- Pointer to the information for this handler
 */
static void* handle_c3(void* me_v) 
{
    // Pointer to the information for the
    struct handler_info* me = reinterpret_cast<struct handler_info*>(me_v);
    
    while (true) {
	sem_clear(&me->sem);

	if (signal_mode == SIGNAL_NORMAL) {
	    relay(me->name, C3_RED, RELAY_OFF);
	    relay(me->name, C3_YELLOW, RELAY_OFF);
	    relay(me->name, C3_GREEN, RELAY_OFF);
	}

	if (sem_wait(&me->sem) != 0) {
	    if (errno == EAGAIN)
		continue;
	    syslog(LOG_ERR, "c3: ERROR: Semaphore failed -- abort ");
	    exit(8);
	}
	if (signal_mode == SIGNAL_LOW_NOISE) continue;

	// Display red
	relay(me->name, C3_RED, RELAY_ON);
	sem_wait_time(&me->sem, C3_WAIT);
	if (signal_mode == SIGNAL_LOW_NOISE) continue;

	// Display yellow
	relay(me->name, C3_RED, RELAY_OFF);
	relay(me->name, C3_YELLOW, RELAY_ON);
	sem_wait_time(&me->sem, C3_WAIT);
	if (signal_mode == SIGNAL_LOW_NOISE) continue;

	// Display green
	relay(me->name, C3_YELLOW, RELAY_OFF);
	relay(me->name, C3_GREEN, RELAY_ON);
	sem_wait_time(&me->sem, C3_WAIT);
	if (signal_mode == SIGNAL_LOW_NOISE) continue;
    }
    return (NULL);
}
/*
 * handle_w4 -- Handle the four white light signal
 *
 * Signal rests on "Yellow"
 * 	Button press moves to red -> yellow -> green -> yellow
 * 	Button press during this sequence advances to the next state
 *
 * Parameters
 * 	me_v -- Pointer to the information for this handler
 */
static void* handle_w4(void* me_v) 
{
    // Pointer to the information for the
    struct handler_info* me = reinterpret_cast<struct handler_info*>(me_v);
    
    while (true) {
	sem_clear(&me->sem);

	relay(me->name, W4_RED, RELAY_OFF);
	relay(me->name, W4_YELLOW, RELAY_ON);
	relay(me->name, W4_GREEN, RELAY_OFF);

	if (sem_wait(&me->sem) != 0) {
	    if (errno == EAGAIN)
		continue;
	    syslog(LOG_ERR, "w4: ERROR: Semaphore failed -- abort ");
	    exit(8);
	}

	// Display red
	relay(me->name, W4_RED, RELAY_ON);
	relay(me->name, W4_YELLOW, RELAY_OFF);
	sem_wait_time(&me->sem, W4_WAIT);

	// Display yellow
	relay(me->name, W4_RED, RELAY_OFF);
	relay(me->name, W4_YELLOW, RELAY_ON);
	sem_wait_time(&me->sem, W4_WAIT);

	// Display green
	relay(me->name, W4_YELLOW, RELAY_OFF);
	relay(me->name, W4_GREEN, RELAY_ON);
	sem_wait_time(&me->sem, W4_WAIT);
    }
    return (NULL);
}
    
/*
 * lamp_test -- Turn on all lamps.  
 */
static void lamp_test(void)
{
    relay("lamp_test", H2_RELAY, RELAY_ON);
    relay("lamp_test", W4_RED, RELAY_ON);
    relay("lamp_test", W4_YELLOW, RELAY_ON);
    relay("lamp_test", W4_GREEN, RELAY_ON);
    relay("lamp_test", C3_RED, RELAY_ON);
    relay("lamp_test", C3_YELLOW, RELAY_ON);
    relay("lamp_test", C3_GREEN, RELAY_ON);
    relay("lamp_test", TRACK_SEM_L, RELAY_ON);
    relay("lamp_test", TRACK_SEM_R, RELAY_ON);
    relay("lamp_test", TRACK_CAR, RELAY_ON);
}

/*
 * do_button  -- Push a button
 *
 * Parameters
 * 	button -- The button to push
 * Returns
 * 	Result of the push
 */
static std::string do_button(const char button)
{
    switch (button) {
	case '1':
	    return (push(HANDLE_H2));
	case '2':
	    return (push(HANDLE_W4));
	case '3':
	case '4':
	case '6':
	    return (push(HANDLE_C3));
	case '5':
	    return (push(HANDLE_CAR));
	case '7':
	    return (push(HANDLE_LWW));
	case '8':
	    return (push(HANDLE_BELL));
	case '9':
	    return (push(HANDLE_UWW));
	case 'n':
	case 'N':
	    return (push(HANDLE_NOISE));
	default:
	    return ("Unknown button");
    }
}

/*
 * status -- print the status of each relay an input
 */
static std::string status(void)
{
    static const char* names[] = {
#define D(x,y) y
	RELAY_LIST
#undef D
    };

    std::ostringstream result;	// Result of the status
    for (int i = 0; i <= LAST_RELAY; ++i) {
	result << "Relay " << i << ":[" << names[i] << "] state " << 
		relay_status(static_cast<enum RELAY_NAME>(i)) << std::endl;
    }
    for (int i = 0; i < 2; ++i) {
	result << "GPIO " << i << " state " << gpio_status(i) << std::endl;
    }
    return (result.str());
}

/*
 * get_ip_address -- Return the IP address of the interface wlan0
 */
static std::string get_ip_address(void)
{
    int fd;		// FD of the interface
    struct ifreq ifr;	// Interface information

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to "eth0" */
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    /* return result */
    return (inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}
/*
 * do_cmd -- Process a command
 */
static std::string do_cmd(const char* const cmd)
{
    switch (cmd[0]) {
	case 'r':
	    relay_reset();
	    break;
	case 'i':
	    return(get_ip_address() + "\n");
	case 't':
	    lamp_test();
	    return ("OK");
	case 'b':
	    return (do_button(cmd[1]));
	case 's':
	    return (status());
	default:
	    return (
		    "s -- Status\n"
		    "r -- reset\n"
		    "i -- IP addr -- to console\n"
		    "t -- lamp test\n"
		    "b<x> -- Push button x\n"
		    "x -- Exit\n");
    }
    // Can never reach here
    abort();
}
/*
 * cmd_line -- Process the command line
 */
static void cmd_line(void)
{
    while (1) {
	std::cout << "Cmd> " << std::flush;

	std::string cmd;	// Command input
	std::getline(std::cin, cmd);

	// Process command, get result
	std::string result = do_cmd(cmd.c_str());
	std::cout << result << std::endl;
    }
}
/*
 * usage -- Tell someone how to use the thing
 */
static void usage(void)
{
    std::cout << "Usage is garden [-v] [-s] [-d] [-r] " << std::endl;
    std::cout << "       -v Verbose " << std::endl;
    std::cout << "       -s Log to stderr and syslog " << std::endl;
    std::cout << "       -d debug " << std::endl;
    std::cout << "       -r Simulate relays " << std::endl;
    exit(8);
}


/*
 * cmd_socket -- process the commands on a socket
 *
 * Parameters
 *     x_client_fd -- The socket we are working on
 */
static void* cmd_socket(void* x_client_id)
{
    // The FD to read as a fd
    int client_fd = reinterpret_cast<long int>(x_client_id);
    while (true) {
	// The command prompt
	static const char PROMPT[] = "Cmd> ";
	write(client_fd, PROMPT , sizeof(PROMPT)-1);

	char buf[50];	// Buffer containing the line we ared

	if (read(client_fd, buf, sizeof(buf)) <= 0) {
	    syslog(LOG_INFO, "Command process %ld exited", pthread_self());
	    close(client_fd);
	    pthread_exit(0);
	}
	if (buf[0] == 'x') {
	    close(client_fd);
	    pthread_exit(0);
	}

	// Execute the command
	std::string result = do_cmd(buf);

	result += '\n';
	if (write(client_fd, result.c_str(), result.length()) != 
		static_cast<ssize_t>(result.length())) {
	    syslog(LOG_INFO, "Command process %ld exited when writing result", 
		    pthread_self());
	    close(client_fd);
	    pthread_exit(0);
	}
    }
}

// The socket that controls this program
static const char* const socket_path = "/tmp/garden.control";
/*
 * start_socket -- Create a socket and a thread to listen to it
 */
static void* start_socket(void*)
{
    struct sockaddr_un addr;	// The address of the socket

    // Open the socket for this process
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
	std::cerr  << "ERROR: Could not create control socket" << std::endl;
	exit(EXIT_FAILURE);
    }

    // Initialize the address
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    unlink(socket_path);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
	syslog(LOG_ERR, "ERROR: Could not bind to socket.  Command processor stopped");
	pthread_exit(0);
    }
    fchmod(fd, 0660);
    chmod(socket_path, 0660);

    // Listen to dhe data for the socket 
    if (listen(fd, 5) == -1) {
	syslog(LOG_ERR, "ERROR: Could not listen to socket.  Command processor stopped");
	pthread_exit(0);
    }

    while (1) {
	// The FD for the client socket
	int client_fd = accept(fd, NULL, NULL);
	if (client_fd < 0) {
	    syslog(LOG_ERR, "ERROR: Accept failed.  Stopping command line processing");
	    pthread_exit(0);
	}
        pthread_t client_id;	// ID of the client we have
	if (pthread_create(&client_id, NULL, cmd_socket, 
		reinterpret_cast<void*>(client_fd))) {
	    syslog(LOG_ERR, "pthread_create failed -- abort");
	    pthread_exit(0);
	}

    }
    return (0);
}

/*
 *  drain -- Dump all input from a fd
 * 
 * Parameters
 *	fd -- FD to drain
 */
static void drain(const int fd) 
{
    // Poll structure so we can get the input
    struct pollfd fds[] = {
	{fd, POLLIN, 0}
    };

    // Loop till no more input
    while (1) {
	if (poll(fds, 1, 0) != 1) {
	    return;
	}
	char input[2];	// Input from the socket

	ssize_t read_size = read(fd, input, 1);
	if (read_size == 0) {
	    syslog(LOG_ERR, "EOF while attempting to drain input");
	    return;
	}
	if (read_size != 1) {
	    syslog(LOG_ERR, "Read erro while draining");
	    exit(EXIT_FAILURE);
	}
    }
}
/*
 * input_thread -- Read the input device and control the relays
 */
static void* input_thread(void*)
{
    mkfifo(INPUT_PIPE, 0666);
    chmod(INPUT_PIPE, 0666);

    // Open the socket for this process
    int fd = open(INPUT_PIPE, O_RDWR);
    if (fd < 0) {
	syslog(LOG_ERR, "ERROR: Could not open input pipe");
	exit(EXIT_FAILURE);
    }
    drain(fd);
    while (1) {
	char input[2];	// Input from the socket

	// Read one character from input, get size
	ssize_t read_size = read(fd, input, 1);
        if (read_size == 0) {
	    syslog(LOG_ERR, "ERROR: EOF seen on input pipe");
	    exit(EXIT_FAILURE);
        }
	if (read_size != 1) {
	    syslog(LOG_ERR, "ERROR: Read error on input pipe");
	    exit(EXIT_FAILURE);
	}
	char& ch = input[0];	// Get the character in a nice variable
	syslog(LOG_NOTICE, "Input character %c", ch);

	if ((ch >= '0') && (ch <= '9')) {
	    // Map the button to what need to be used
	    int handler_index = button_handler_map[ch - '0'];
	    if (handler_index < HANDLE_LAST) {
		if (sem_post(&handler_array[handler_index].sem) == -1) {
		    syslog(LOG_ERR, "ERROR: sem_post failed -- abort");
		    exit(EXIT_FAILURE);
		}
	    }
	} else {
	    syslog(LOG_INFO, "Bad input character %c", input[0]);
	}
    }
}

int main(int argc, char *argv[])
{
    try {
	bool stdout_log = false;	// Send log messages to stdout
	//	-- v verbose
	//	-- s log to stdout
	//	-- d Debug -- stay in foreground
	//	-- r Simulate relays
	int opt;	// Option we are looking
	while ((opt = getopt(argc, argv, "vsdr")) != -1) {
	    switch (opt) {
		case 'v':
		    verbose = true;
		    break;
		case 's':
		    stdout_log = true;
		    break;
		case 'd':
		    debug = true;
		    break;
		case 'r':
		    simulate = true;
		    break;
		default: /* '?' */
		    usage();
	    }
	}

	if (optind < argc) {
	    std::cerr << "Extra arguements on the command line" << std::endl;
	    exit(EXIT_FAILURE);
	}

	// Open up the syslog system
	openlog("garden", stdout_log ? LOG_PERROR : 0, LOG_USER); 

	relay_setup();
	relay_reset();

	pthread_t socket_id;	// ID number of the handler
	if (pthread_create(&socket_id, NULL, start_socket, NULL)) {
	    syslog(LOG_ERR, "pthread_create failed -- abort");
	    exit(8);
	}
	pthread_t input_id;	// ID number of the handler
	if (pthread_create(&input_id, NULL, input_thread, NULL)) {
	    syslog(LOG_ERR, "pthread_create failed for input thread-- abort");
	    exit(8);
	}

	// Loop through each handler and start it
	for(int id = HANDLE_FIRST; id < HANDLE_LAST; ++id) {
	    if (sem_init(&handler_array[id].sem, 0, 0) == -1) {
		syslog(LOG_ERR, "sem_init failed -- abort");
		exit(8);
	    }

	    pthread_t handler_id;	// ID number of the handler
	    if (pthread_create(&handler_id, NULL, handler_array[id].funct, &handler_array[id])) {
		syslog(LOG_ERR, "pthread_create failed -- abort");
		exit(8);
	    }
	}
#if 0
	if (!debug) {
	    daemon(true, false);	// Turn into a daemon
	}
#endif

	// Do we need to start a command line interface?
	if (debug) {
	    cmd_line();
	} else {
	    while (1) {
		wait(NULL);
	    }
	}
	exit(0);
    }
    catch (relay_error& error) {
	std::cout << "Relay exception: " << error.error << std::endl;
	exit(8);
    }
}
