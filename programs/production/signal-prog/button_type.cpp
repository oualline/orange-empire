/*
 * button_type -- Simulate buttons by typing on the terminal
 */
#include <iostream>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#include "device.h"


int main()
{
    // Open the socket for this process
    int fd = open(INPUT_PIPE, O_WRONLY);
    if (fd < 0) {
	std::cerr  << "ERROR: Could not open input pipe" << std::endl;
	exit(EXIT_FAILURE);
    }
    struct termios term_flags;	// Flags we are using for raw input
    struct termios old_flags;	// Flags we will exit with
    if (tcgetattr(0, &term_flags) != 0) {
	std::cout << "ERROR: Could not get terminal attributes\n";
	exit(EXIT_FAILURE);
    }
    old_flags = term_flags;

    cfmakeraw(&term_flags);
    term_flags.c_lflag |= ECHO;
    if (tcsetattr(0, 0, &term_flags) != 0) {
	std::cout << "ERROR: Could not set terminal attributes\n";
	exit(EXIT_FAILURE);
    }

    while (1) {
	char input[2];	// Input from the socket

	if (read(0, input, 1) != 1) {
	    std::cerr << "ERROR: Read error " << std::endl;
	    if (tcsetattr(0, 0, &old_flags) != 0) {
		std::cout << "ERROR: Could not set terminal attributes\n";
		exit(EXIT_FAILURE);
	    }
	    exit(EXIT_FAILURE);
	}
	if ((input[0] == 'x') || (input[0] == '\003')) {
	    if (tcsetattr(0, 0, &old_flags) != 0) {
		std::cout << "ERROR: Could not set terminal attributes\n";
		exit(EXIT_FAILURE);
	    }
	    exit(0);
	}

	write(fd, input, 1);
    }
    return (0);
}

