all: gpio-test 

gpio-test: gpio-test.cpp
	g++ -g -Wall -Wextra -o gpio-test gpio-test.cpp -lwiringPi

clean:
	rm -f gpio-test
