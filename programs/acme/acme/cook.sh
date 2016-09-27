#
# Cook all the images in raw into .fb names
#
TTY=`tty`
FILES=`(cd raw;echo *)`
for i in $FILES; do
    echo $i
    #setterm -term linux -blank poke >/dev/tty1 </dev/tty1
    name=`echo $i | sed -e 's/\.[^\.]*$//'`
    #fbi --autozoom --noverbose raw/$i --device /dev/fb0 --noedit </dev/tty1 >/dev/tty1 &
    fbi --autozoom --noverbose raw/$i --device /dev/fb0 --noedit  <$TTY >$TTY&
    sleep 10
    cp /dev/fb0 cooked/$name.fb
    killall fbi
    sleep 1
done
chown pi:pi cooked/*
