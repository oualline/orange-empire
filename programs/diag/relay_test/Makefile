all: relay_test

HEADER=../../production/signal-prog/
LIB_MOD=../../production/signal-prog/relay.cpp

relay_test: relay_test.cpp $(LIB_MOD)
	g++ -DGARDEN_RELAYS -g -std=c++11 -Wall -Wextra -I$(HEADER) -o relay_test relay_test.cpp $(LIB_MOD)

clean: 
	rm -f relay_test
