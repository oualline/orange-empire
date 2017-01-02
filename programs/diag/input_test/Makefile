all: input_test

HEADER=../../production/signal-prog/

input_test: input_test.cpp
	g++ -g -Wall -Wextra -I${HEADER} -o input_test input_test.cpp 

clean: 
	rm -f input_test
