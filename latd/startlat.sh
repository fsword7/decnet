#!/bin/sh
#
# startlat.sh
#
# Starts/Stops latd process
#
# This script should go in /etc/init.d      (Debian)
#                          /etc/rc.d/init.d (RedHat)
#                          /sbin/init.d     (SuSE)
#
# and you should link to it from the relevant runlevel startup directory
# eg: (Debian)
#      ln -s /etc/init.d/startlat.sh /etc/rc2.d/S79lat.sh
#
#     (RedHat)
#      ln -s /etc/rc.d/init.d/startlat.sh /etc/rc.d/rc3.d/S79lat
#
#     (SuSE)
#      ln -s /sbin/init.d/startlat.sh /sbin/init.d/rc2.d/S79lat
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
