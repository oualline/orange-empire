bash common.sh

apt-get -qq -y install vim i2c-tools libusb-1.0-0-dev wiringpi screen rsync

	# This appears to not be needed as this should work
	#rm -f /etc/init.d/hwclock.sh
	#mv ./etc/init.d/hwclock.sh /etc/init.d

update-rc.d hwclock.sh enable

	#if ( grep -s /etc/modules-load.d/modules.conf rtc-ds1307 ) ; then
	#    echo "modules.conf correct"
	#else
	#    echo "rtc-ds1307" >>  /etc/modules-load.d/modules.conf 
	#fi

mkdir -p /etc/conf.d
mv /root/etc/rtc-i2c           /etc/conf.d/rtc-i2c 
mv /root/etc/rtc-i2c.conf      /etc/modules-load.d/rtc-i2c.conf 
mv /root/etc/rtc-i2c.rules     /etc/udev/rules.d/rtc-i2c.rules 
mv /root/etc/rtc-i2c.service   /lib/systemd/system/rtc-i2c.service

sed -i -e 's/#dtparam=i2c_arm=on/dtparam=i2c_arm=on/' /boot/config.txt

systemctl daemon-reload
systemctl enable rtc-i2c

# 
su -c "(cd /home/pi/orange/programs/production;make;make install)" pi
