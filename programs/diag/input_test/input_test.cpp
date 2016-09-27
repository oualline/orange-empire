/*
 * input_test -- Read data from the input socket and print it
 */
#include <iostream>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <poll.h>

#include "device.h"

static void drain(const int fd) 
{
    struct pollfd fds[] = {
	{fd, POLLIN, 0}
    };
    while (1) {
	if (poll(fds, 1, 0) != 1) {
	    return;
	}
	char input[2];	// Input from the socket

	ssize_t read_size = read(fd, input, 1);
	if (read_size == 0) {
	    std::cerr << "EOF" << std::endl;
	    continue;
	}
	if (read_size != 1) {
	    std::cerr << "ERROR: Read error / read_size " << read_size << 
		" errno:" << errno << std::endl;
	    perror("READ:");
	    exit(EXIT_FAILURE);
	}
	write(1, input, 1);
    }
}
int main()
{
    mkfifo(INPUT_PIPE, 0666);
    chmod(INPUT_PIPE, 0666);

    // Open the socket for this process
    int fd = open(INPUT_PIPE, O_RDWR);
    if (fd < 0) {
	std::cerr  << "ERROR: Could not open input pipe" << std::endl;
	exit(EXIT_FAILURE);
    }
    std::cout << "Drain " << std::endl;
    drain(fd);
    std::cout << std::endl << "Drain Done " << std::endl;

    while (1) {
	char input[2];	// Input from the socket

	ssize_t read_size = read(fd, input, 1);
        if (read_size == 0) {
	    std::cerr << "EOF" << std::endl;
	    continue;
        }
	if (read_size != 1) {
	    std::cerr << "ERROR: Read error / read_size " << read_size << 
		" errno:" << errno << std::endl;
	    perror("READ:");
	    exit(EXIT_FAILURE);
	}
	write(1, input, 1);
    }
    return (0);
}

