all: relay_test

HEADER=../../production/signal-prog/
LIB_MOD=../../production/signal-prog/relay.cpp

relay_test: relay_test.cpp $(LIB_MOD)
	g++ -g -Wall -Wextra -I$(HEADER) -o relay_test relay_test.cpp $(LIB_MOD)

clean: 
	rm relay_test
