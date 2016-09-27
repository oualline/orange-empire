if [ `tty` = "/dev/tty1" ] ; then
    bash /home/garden/bin/setup.sh
    if [ ! -f no-run ] ; then
       ./bin/master
    fi
fi
