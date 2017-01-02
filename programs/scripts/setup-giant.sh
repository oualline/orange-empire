if [ ! -f /mnt/usb/part2.tar.gz ] ; then
    echo "File /mnt/usb/part2.tar.gz must be there"
    exit 8
fi

if [ $PWD != /root/scripts ] ; then
    echo "Must be run from /root/scripts"
    exit 8
fi

# Note we keep garden.rules because it's used give the correct permissions 
# to the realy board
time zcat /mnt/usb/part2.tar.gz | tar xvf - ./etc/network/interfaces ./etc/wpa_supplicant/wpa_supplicant.conf ./etc/udev/rules.d/99-garden.rules 

if [ ! -f /etc/wpa_supplicant/wpa_supplicant.conf.old ] ; 
    then mv /etc/wpa_supplicant/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf.old ; 
fi

if [ ! -f /etc/network/interfaces.old ] ; then 
    mv /etc/network/interfaces /etc/network/interfaces.old ; 
fi

mv ./etc/udev/rules.d/99-garden.rules /etc/udev/rules.d/
mv ./etc/network/interfaces /etc/network
mv ./etc/wpa_supplicant/wpa_supplicant.conf /etc/wpa_supplicant
ifdown wlan0
ifup wlan0


apt-get -qq update;apt-get -qq -y upgrade
apt-get -qq -y install vim i2c-tools libusb-1.0-0-dev wiringpi mpg321 fbi screen
apt-get -qq -y install daemon

addgroup --gid 1004 garden
adduser --disabled-password --gecos "" --uid 1001 --gid 1004 garden
usermod -G input garden
usermod -G dialout garden

chpasswd <<EOF
pi:Small+Power
garden:Big/Boy
root:Station!Master
EOF

zcat /mnt/usb/part2.tar.gz | (cd /;tar xf - --keep-old-files ./home/garden ./home/pi)

# Giant has hardware clock
if true ; then
    zcat /mnt/usb/part2.tar.gz | tar xvf -  \
	    ./etc/conf.d/rtc-i2c	\
	    ./etc/modules-load.d/rtc-i2c.conf \
	    ./etc/udev/rules.d/rtc-i2c.rules \
	    ./lib/systemd/system/rtc-i2c.service

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
    mv ./etc/conf.d/rtc-i2c                 /etc/conf.d/rtc-i2c 
    mv ./etc/modules-load.d/rtc-i2c.conf    /etc/modules-load.d/rtc-i2c.conf 
    mv ./etc/udev/rules.d/rtc-i2c.rules     /etc/udev/rules.d/rtc-i2c.rules 
    mv ./lib/systemd/system/rtc-i2c.service /lib/systemd/system/rtc-i2c.service

    sed -i -e 's/#dtparam=i2c_arm=on/dtparam=i2c_arm=on/' /boot/config.txt

    systemctl daemon-reload
    systemctl enable rtc-i2c
fi

# 
# Do not make the garden programs on the acme
#su -c "(cd /home/pi/orange/programs/giant;make;make install)" pi

