setterm --blank poke >/dev/tty0
#fbi --autozoom --noverbose -T 0 cooked/p1.png --device /dev/fb0 </dev/tty0 >/dev/tty0
fbi --autozoom --noverbose cooked/p1.png --device /dev/fb0 --noedit </dev/tty0 >/dev/tty0
