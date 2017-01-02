. common.sh

apt-get -qq -y install vim i2c-tools libusb-1.0-0-dev wiringpi mpg321 fbi screen rsync

sed -i -e 's/autologin pi/autologin garden/' /etc/systemd/system/autologin@.service
systemctl set-default multi-user.target
ln -fs /etc/systemd/system/autologin@.service /etc/systemd/system/getty.target.wants/getty@tty1.service

# 
# Do not make the garden programs on the acme
#su -c "(cd /home/pi/orange/programs/production;make;make install)" pi

# Don't do this yet as we are still working on it
su -c "(cd /home/pi/orange/programs.acme/make;make install)" pi
usermod -G video pi
usermod -G video garden
usermod -G audio garden
usermod -G audio pi
