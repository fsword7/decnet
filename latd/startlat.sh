#!/bin/sh
#
# startlat.sh
#
# Starts/Stops latd process
#
# chkconfig: - 79 79
# description: latd
# processname: latd
# config: /etc/latd.conf
#
# This script should go in /etc/rc.d/init.d
#
# Yu can install it on a Red Hat system with:
#
# chkconfig --level 345 latd on
# -----------------------------------------------------------------------------
#

#
# See which distribution we are using and customise the start/stop 
# commands and the console display.
#
if [ -d /var/lib/YaST ]
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
LATCP=""
[ -x /usr/sbin/latcp ] && LATCP="/usr/sbin/latcp"
[ -x /usr/local/sbin/latcp ] && LATCP="/usr/local/sbin/latcp"

if [ -z "$LATCP" ]
then
  echo "Cannot find latcp."
  exit 1
fi

case $1 in
   start)
     echo -n "Starting LAT: "
     $LATCP -s
     STATUS=$?
     echo "$startendecho"
     ;;

   stop)
     echo -n "Stopping LAT... "
     $LATCP -h
     STATUS=$?
     echo "$stopendecho"
     ;;

   restart|reload)
     echo -n "Restarting LAT: "
     $LATCP -h
     $LATCP -s
     STATUS=$?
     echo -n "$startecho"
     echo "$stopendecho"
     ;;

   *)
     echo "Usage $0 {start|stop|restart}"
     STATUS=0;
     ;;
esac

exit $STATUS
