
PROGS= perms

CXX=g++
CXXFLAGS=-Wall -Wextra -ggdb

all: $(PROGS)

install: all
	sudo cp $(PROGS) /home/garden/bin
	sudo chown root:root /home/garden/bin/perms
	sudo chmod ug+s /home/garden/bin/perms

perms: perms.cpp 
	$(CXX) $(CXXFLAGS) -o perms perms.cpp
		
clean:
	rm -f perms
