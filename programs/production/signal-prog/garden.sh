#!/bin/sh
p=`pidof garden`
if [ x$p = x ] ; then
    echo "garden program is not running"
    echo -n "Press enter"
    read x
else
    socat - UNIX-CONNECT:/tmp/garden.control
fi
