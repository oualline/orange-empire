SRCS=giant.cpp ../../production/signal-prog/relay.cpp all-off.cpp

CFLAGS=-DGIANT_RELAYS -g -Wall -Wextra -I../../production/signal-prog -std=c++11

all: all-off giant 

all-off:all-off.o relay.o
	g++ $(CFLAGS) -o all-off all-off.o relay.o

GIANT_OBJS=giant.o relay.o 
giant: $(GIANT_OBJS)
	g++ $(CFLAGS) -o giant $(GIANT_OBJS) -lpthread -lwiringPi

giant.o: giant.cpp 
	g++ $(CFLAGS) -c giant.cpp

relay.o: ../../production/signal-prog/relay.cpp
	g++ $(CFLAGS) -c ../../production/signal-prog/relay.cpp

DESTDIR=/home/garden/bin

install: giant all-off
	-sudo killall gaint 
	sudo cp giant all-off /home/garden/bin
	sudo chown root $(DESTDIR)/giant
	sudo chmod u+s $(DESTDIR)/giant
	@echo "Do a make install-etc to install etc files"

install-etc:
	sudo cp etc/giant /etc/init.d/
	sudo update-rc.d giant defaults 88

clean:
	rm -f *.o *.d all-off giant

%.d:%.cpp
	g++ $(CFLAGS) -MM $*.cpp > $*.d

%.o:%.cpp
	g++ -c $(CFLAGS) $*.cpp 


-include $(SRCS:.cpp=.d)
