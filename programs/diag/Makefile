DIRS= relay_test input_test button power_test

all:
	@for i in $(DIRS); do echo "==== $$i";(cd $$i;make all);done

clean:
	@for i in $(DIRS); do echo "==== $$i";(cd $$i;make clean);done

install:
	@for i in $(DIRS); do echo "==== $$i";(cd $$i;make install);done
