all: keep_running

keep_running: keep_running.cpp
	g++ -ggdb -Wall -Wextra -o keep_running keep_running.cpp

install: keep_running
	sudo cp keep_running /home/garden/bin
	
clean: 
	rm -f keep_running
