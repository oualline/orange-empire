/********************************************************
 * keep_running -p<pid-file> <cmd> <args>
 *
 * Keep a process running.  If it aborts, start it again.
 ********************************************************/
#include <iostream>
#include <fstream>

#include <string>

#include <cstring>
#include <csignal>

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <syslog.h>
#include <sys/param.h>

static pid_t pid = 0;	// Pid of our child process
/*
 * handle term term signal -- Send it to process of the command
 *   	we are running 
 */
static void term(int)
{
    if (pid != 0)
	kill(pid, SIGTERM);
    exit(0);
}

int main(int argc, char* argv[])
{
    if (argc <= 2) {
	std::cerr << "Usage " << argv[0] << " -p<pid-file> <cmd> <argc>" << std::endl;
	exit(8);
    }
    if (strncmp(argv[1], "-p", 2) != 0) {
	std::cerr << "Usage " << argv[0] << " -p<pid-file> <cmd> <argc>" << std::endl;
	exit(8);
    }
    char pid_file[MAXPATHLEN];	// File to put the pid in
    strncpy(pid_file, &argv[1][2], sizeof(pid_file));
    pid_file[sizeof(pid_file)-1] = '\0';

    signal(SIGTERM, term);
    daemon (1, 1);

    std::ofstream pid_stream(pid_file);	// Open the pid file
    if (!pid_stream.bad()) {
	pid_stream << getpid() << std::endl;
	pid_stream.close();
    }
    std::string name("keep_running:");	// The name we will use for syslog
    name += argv[2];
    openlog(name.c_str(), 0, LOG_USER);

    while (1) {
	syslog(LOG_INFO, "Starting %s", argv[2]);
	// Produce child
	pid = fork();
	if (pid == 0) {
	    execvp(argv[2], &argv[2]);
	    exit(8);
	}
	int status = 0;	// Status of the child
	if (waitpid(pid, &status, 0) != pid)
	    exit(8);

	syslog(LOG_NOTICE, "Program %s Exited %s %d", argv[2], 
		WIFEXITED(status) ? "normally" : "aborted",
		WEXITSTATUS(status));
	bool bad = false;
	if (!WIFEXITED(status))
	    bad = true;
	else {
	    if (WEXITSTATUS(status) != 0)
		bad = true;
	}

	if (bad)
	    sleep(30);
    }
}
