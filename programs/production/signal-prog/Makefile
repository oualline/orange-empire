PROGS= garden button_type button_mcp button_avr

CXX=g++
CXXFLAGS=-Wall -Wextra -ggdb

all: $(PROGS) 

install: all
	sudo cp $(PROGS) /home/garden/bin
	sudo cp garden.sh /home/garden/bin
	sudo chown garden:garden /home/garden/bin/garden
	sudo chmod ug+s /home/garden/bin/garden
	sudo chown garden:garden /home/garden/bin/button_mcp
	sudo chmod ug+s /home/garden/bin/button_mcp
	#
	sudo chown garden:input /home/garden/bin/button_avr
	sudo chmod 555 /home/garden/bin/button_avr
	sudo chmod ug+s /home/garden/bin/button_avr
	#
	echo "Do a make install-etc to install etc files"

install-etc:
	#sudo cp etc/garden /etc/init.d/
	#sudo update-rc.d garden defaults 88
	#sudo cp etc/button_mcp /etc/init.d/
	#sudo update-rc.d button_mcp defaults 87
	sudo cp udev/99-garden.rules /etc/udev/rules.d/

button_type: button_type.cpp device.h
	$(CXX) $(CXXFLAGS) -o button_type button_type.cpp

button_mcp: button_mcp.cpp device.h
	$(CXX) $(CXXFLAGS) -o button_mcp button_mcp.cpp -lusb-1.0

button_avr: button_avr.cpp device.h
	$(CXX) $(CXXFLAGS) -o button_avr button_avr.cpp -lusb-1.0

garden: garden.cpp relay.cpp relay.h device.h
	$(CXX) $(CXXFLAGS) -o garden garden.cpp relay.cpp -lrt -lpthread

process-key: process-key.cpp
	$(CXX) $(CXXFLAGS) -o process-key process-key.cpp

read-key: read-key.cpp
	$(CXX) $(CXXFLAGS) -o read-key read-key.cpp

clean:
	rm -f $(PROGS)
