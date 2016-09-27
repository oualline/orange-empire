if [ ! -f /mnt/usb/part2.tar.gz ] ; then
    echo "File /mnt/usb/part2.tar.gz must be there"
    exit 8
fi

if [ $PWD != /root/scripts ] ; then
    echo "Must be run from /root/scripts"
    exit 8
fi

# Note we keep garden.rules because it's used to identify the relay board on 
# the acme
time zcat /mnt/usb/part2.tar.gz | tar xvf - ./root/etc 

if [ ! -f /etc/wpa_supplicant/wpa_supplicant.conf.old ] ; 
    then mv /etc/wpa_supplicant/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf.old ; 
fi

if [ ! -f /etc/network/interfaces.old ] ; then 
    mv /etc/network/interfaces /etc/network/interfaces.old ; 
fi

mv ./root/etc/99-garden.rules /etc/udev/rules.d/
mv ./root/etc/interfaces /etc/network
mv ./root/etc/wpa_supplicant.conf /etc/wpa_supplicant
ifdown wlan0
ifup wlan0

time apt-get -qq update;time apt-get -qq -y upgrade

addgroup --gid 1004 garden
adduser --disabled-password --gecos "" --uid 1001 --gid 1004 garden
usermod -G input garden
usermod -G dialout garden

addgroup --gid 1005 docent
adduser --disabled-password --gecos "" --uid 1002 --gid 1005 docent

addgroup --gid 1006 reboot
adduser --disabled-password --gecos "" --uid 1003 --gid 1006 reboot

addgroup --gid 1007 date
adduser --disabled-password --gecos "" --uid 1004 --gid 1007 date

bash /root/setup/password.sh

