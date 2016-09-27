deluser garden
deluser docent
deluser reboot
deluser date

delgroup garden
delgroup docent
delgroup reboot
delgroup date

mkdir -p /home/old
if [ -d /home/garden ] ; then mv /home/garden /home/old; fi
if [ -d /home/docent ] ; then mv /home/docent /home/old; fi
if [ -d /home/reboot ] ; then mv /home/reboot /home/old; fi
if [ -d /home/date ] ; then mv /home/date /home/old; fi
