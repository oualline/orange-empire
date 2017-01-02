SRCS=acme.cpp hw.cpp master.cpp ../../production/signal-prog/relay.cpp long-demo.cpp wind-demo.cpp short-demo.cpp all-off.cpp demo-common.cpp

CFLAGS=-DACME_RELAYS -g -Wall -Wextra -I../../production/signal-prog -std=c++11

all: all-off acme master long-demo short-demo wind-demo

all-off:all-off.o relay.o
	g++ $(CFLAGS) -o all-off all-off.o relay.o

lcd-on:lcd-on.o  
	g++ $(CFLAGS) -o lcd-on lcd-on.o  
	sudo chown root lcd-on
	sudo chmod u+s lcd-on

lcd-off:lcd-off.o 
	g++ $(CFLAGS) -o lcd-off lcd-off.o 
	sudo chown root lcd-off
	sudo chmod u+s lcd-off

LONG_OBJS =long-demo.o relay.o hw.o common.o demo-common.o
long-demo: $(LONG_OBJS)
	g++ $(CFLAGS) -o long-demo $(LONG_OBJS) -lwiringPi
	sudo chown root long-demo
	sudo chmod u+s long-demo

SHORT_OBJS = short-demo.o relay.o hw.o common.o demo-common.o
short-demo: $(SHORT_OBJS)
	g++ $(CFLAGS) -o short-demo $(SHORT_OBJS) -lwiringPi
	sudo chown root short-demo
	sudo chmod u+s short-demo

WIND_OBJS =wind-demo.o relay.o hw.o common.o demo-common.o
wind-demo: $(WIND_OBJS)
	g++ $(CFLAGS) -o wind-demo $(WIND_OBJS) -lwiringPi
	sudo chown root wind-demo
	sudo chmod u+s wind-demo

MASTER_OBJS=master.o relay.o common.o
master: $(MASTER_OBJS)
	g++ $(CFLAGS) -o master $(MASTER_OBJS) -lwiringPi 
	sudo chown root master
	sudo chmod u+s master

DEMO_OBJS=demo.o relay.o common.o
demo: $(DEMO_OBJS)
	g++ $(CFLAGS) -o demo $(DEMO_OBJS) -lwiringPi -lpthread
	sudo chown root demo
	sudo chmod u+s demo

ACME_OBJS=acme.o relay.o common.o hw.o
acme: $(ACME_OBJS)
	g++ $(CFLAGS) -o acme $(ACME_OBJS) -lpthread

acme.o: acme.cpp buttons.h
	g++ $(CFLAGS) -c acme.cpp

demo.o: demo.cpp buttons.h
	g++ $(CFLAGS) -c demo.cpp

relay.o: ../../production/signal-prog/relay.cpp
	g++ $(CFLAGS) -c ../../production/signal-prog/relay.cpp

DESTDIR=/home/garden/bin
install: acme master wind-demo short-demo long-demo setup.sh first.sh acme.conf vol_cmd.sh
	-sudo killall acme master wind-demo short-demo long-demo 
	sudo cp setup.sh first.sh acme master wind-demo short-demo long-demo vol_cmd.sh /home/garden/bin
	sudo chown root $(DESTDIR)/master
	sudo chmod u+s $(DESTDIR)/master
	sudo chown root $(DESTDIR)/long-demo
	sudo chmod u+s $(DESTDIR)/long-demo
	sudo chown root $(DESTDIR)/short-demo
	sudo chmod u+s $(DESTDIR)/short-demo
	sudo chown root $(DESTDIR)/wind-demo
	sudo chmod u+s $(DESTDIR)/wind-demo
	sudo cp acme.conf /home/garden
	sudo chown garden:garden /home/garden/acme.conf
	sudo cp -r cooked /home/garden
	sudo chown garden:garden /home/garden/cooked /home/garden/cooked/*
	sudo cp -r acme.sound /home/garden
	sudo chown garden:garden /home/garden/acme.sound /home/garden/acme.sound/*

clean:
	rm -f *.o *.d all-off acme master long-demo short-demo wind-demo

%.d:%.cpp
	g++ $(CFLAGS) -MM $*.cpp > $*.d

%.o:%.cpp
	g++ -c $(CFLAGS) $*.cpp 


-include $(SRCS:.cpp=.d)
