#!/bin/sh
#
# dnprogs.sh 
#
# Starts/Stops DECnet processes
#
# This script MUST be run before TCP/IP is started.
#
# -----------------------------------------------------------------------------
#
FLAGS="start 39 S .  stop 11 1 ."
#
# Daemons to start. You may remove the ones you don't want
#
prefix=/usr
daemons="dnetd phoned"

#
# the -hw flag to startnet tells it to set the hardware address of the ethernet
# card to match the DECnet node address. 
#
startnet="$prefix/sbin/startnet -hw"

case $1 in
   start)
     if [ -f /etc/decnet.conf -a -f /proc/net/decnet ]
     then
       echo -n "Starting DECnet: "
 
       # Run startnet only if we need to
       EXEC=`cat /proc/net/decnet | sed -n '2s/ *\([0-9]\.[0-9]\).*[0-9]\.[0-9]/\1/p'`
       if [ ! -f /proc/net/decnet_neigh -a "$EXEC" = "0.0" ]
       then
         $startnet
         if [ $? != 0 ]
         then
           echo error starting socket layer.
           exit 1
         fi
       fi

       for i in $daemons
       do
         start-stop-daemon --start --quiet --exec $prefix/sbin/$i
         echo -n " $i"
       done
       echo "."
     else
       echo "DECnet not started as it is not configured."
     fi
     ;;

   stop)
     echo -n "Stopping DECnet... "
     for i in $daemons
     do
       start-stop-daemon --stop --quiet --exec $prefix/sbin/$i
     done
     echo "done."
     ;;

   restart|reload|force-reload)
     echo -n "Restarting DECnet: "
     for i in $daemons
     do
       start-stop-daemon --stop --quiet --exec $prefix/sbin/$i
       start-stop-daemon --start --quiet --exec $prefix/sbin/$i
       echo -n " $i"
     done
     echo " done."
     ;;

   *)
     echo "Usage $0 {start|stop|restart|force-reload}"
     ;;
esac

exit 0
