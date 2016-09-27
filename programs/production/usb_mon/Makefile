all: usb_mon.pl

install: usb_mon.pl
	sudo cp usb_mon.pl /home/garden/bin
	sudo chmod a+x /home/garden/bin/usb_mon.pl
	sudo cp usb_mon /etc/init.d
	sudo chmod a+x /etc/init.d/usb_mon
	sudo /sbin/insserv -d usb_mon 
	
clean: 
	@echo Nothing needed
