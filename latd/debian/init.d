#!/bin/sh
#
# Starts/Stops latd process
#
# -----------------------------------------------------------------------------
#

#
# See which distribution we are using and customise the start/stop 
# commands and the console display.
#
if [ -d /etc/debian_version ]
then
  # Debian
  startecho="\$i"
  startendecho="."
  stopendecho="done."
elif [ -d /var/lib/YaST ]
then
  # SuSE
  startendecho=""
  stopendecho="done."
else
  # Assume RedHat
  startendecho=""
  stopendecho="done."
fi

#
# Look for latcp
#
LATCP="/usr/sbin/latcp"

if [ ! -f "$LATCP" ]
then
  exit 1
fi

case $1 in
   start)
     echo -n "Starting LAT: "
     $LATCP -s
     STATUS=$?
     echo "latd."
     ;;

   stop)
     echo -n "Stopping LAT... "
     $LATCP -h
     STATUS=$?
     echo "done."
     ;;

   restart|reload|force-reload)
     echo -n "Restarting LAT: "
     $LATCP -h
     $LATCP -s
     STATUS=$?
     echo "done."
     ;;

   *)
     echo "Usage $0 {start|stop|restart|force-reload}"
     STATUS=0;
     ;;
esac

exit $STATUS
