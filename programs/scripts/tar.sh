if [ ! -f /mnt/usb/part2.tar.gz ] ; then
    echo "File /mnt/usb/part2.tar.gz must be there"
    exit 8
fi

if [ $PWD != /root/scripts ] ; then
    echo "Must be run from /root/scripts"
    exit 8
fi
(cd / ; tar cf /mnt/usb/part2.tar.gz --gzip --one-file-system .)
rsync -avP --delete /home /mnt/usb/orange
rsync -avP --delete --exclude /home/pi/ACME_DVD --delete-excluded /home /mnt/usb/orange
