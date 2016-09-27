all: p_test s_test

install: 

p_test: p_test.cpp
	g++ -g -Wall -Wextra -o p_test p_test.cpp -lwiringPi

s_test: s_test.cpp
	g++ -g -Wall -Wextra -o s_test s_test.cpp -lwiringPi

clean:
	rm -f p_test s_test
