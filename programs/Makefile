SUBDIRS=acme diag private.example rtc scripts

all:
	for i in $(SUBDIRS); do (cd $$i;make all); done

install:
	for i in $(SUBDIRS); do (cd $$i;make install); done

clean:
	for i in $(SUBDIRS); do (cd $$i;make clean); done
