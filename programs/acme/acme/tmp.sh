TTY=`tty`
    fbi --autozoom --noverbose raw/acme-on-street.jpg --device /dev/fb0 --noedit >$TTY 2>$TTY <$TTY &
