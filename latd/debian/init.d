#!/bin/sh
#
# Starts/Stops latd process
#
# -----------------------------------------------------------------------------
#

#
# Look for latcp
#
LATCP="/usr/sbin/latcp"

test -f "$LATCP" || exit 0

case $1 in
   start)
     echo -n "Starting LAT: latd"
     $LATCP -s
     STATUS=$?
     echo "."
     ;;

   stop)
     echo -n "Stopping LAT: latd"
     $LATCP -h
     STATUS=$?
     echo "."
     ;;

   restart|force-reload)
     echo -n "Restarting LAT: latd"
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
