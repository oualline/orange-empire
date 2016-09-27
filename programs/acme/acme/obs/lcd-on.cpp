/*
 * Run the ACME signals and 
 *
 * Run the program and and the signal works
 */
#include <iostream>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

static bool verbose = false;	// Chatter
static bool simulate = false;	// Don't really do it

/********************************************************
 * Perform a system command verbosely
 *
 * Parameters
 * 	command Command to execute
 *********************************************************/
static void do_system(const char* const command)
{
    if (verbose)
	std::cout << "do_system(" << command << ")\r" << std::endl;
    if (simulate)
	return;
    system(command);
}

/********************************************************
 * usage -- Tell the user how to use us
 ********************************************************/
static void usage(void)
{
    std::cout << "Usage lcd-off [-s] [-v] " << std::endl;
    exit(8);
}

int main(int argc, char* argv[])
{
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

    // Fork off a process to turn on the tty
    pid_t pid = fork();
    if (pid == 0) {
	int fd = open("/dev/tty0", O_RDWR);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	do_system("TERM=linux; setterm --blank poke");
    }
    waitpid(pid, NULL, 0);
    do_system("tvservice -p >/dev/null");
    do_system("chvt 2");
    do_system("chvt 1");
}
