all: power 

install: power
	sudo cp power /home/garden/bin/
	sudo cp etc/power /etc/init.d
	sudo chmod a+x /etc/init.d/power
	sudo /sbin/insserv -d power 

power: power.cpp
	g++ -g -Wall -Wextra -o power power.cpp -lwiringPi

clean:
	rm -f power
