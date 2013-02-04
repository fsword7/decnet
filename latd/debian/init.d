#!/bin/sh
#
# Starts/Stops latd process
#
### BEGIN INIT INFO
# Provides:          latd
# Required-Start:    $network $remote_fs
# Required-Stop:     $network $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start the LAT daemon
# Description:       Starts the Local Area Transport daemon to receive
#                    incoming requests and mediate outgoing ones.
### END INIT INFO
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
