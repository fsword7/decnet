#!/bin/sh
#
# dnprogs.sh 
#
# Sets up the ethernet interface(s) and starts/stops DECnet processes
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
# Interfaces to set the MAC address of. If none are specified after
# -hw then all available ethernet interfaces will have their MAC address set
# the the DECnet address. If you do not want to do that (or don't want to do 
# it here) then remove the -hw switch from the command.
# Remember, the MAC address MUST be set for DECnet to work.
#
interfaces="-hw "

startnet="$prefix/sbin/startnet $interfaces"

case $1 in
   start)
     if [ ! -f /etc/decnet.conf ]
     then
       echo "DECnet not started as it is not configured."
       exit 1
     fi

     # If there is no DECnet in the kernel then try to load it.
     if [ ! -f /proc/net/decnet ]
     then
       modprobe decnet
       if [ ! -f /proc/net/decnet ]
       then
         echo "DECnet not started as it is not in the kernel."
	 exit 1
       fi
     fi

     echo -n "Starting DECnet: "

     # Run startnet only if we need to
     EXEC=`cat /proc/net/decnet | sed -n '2s/ *\([0-9]\.[0-9]\).*[0-9]\.[0-9]/\1/p'`
     if [ -z "$EXEC" -o "$EXEC" = "0.0" ]
     then
       $startnet
     fi

     for i in $daemons
     do
       start-stop-daemon --start --quiet --exec $prefix/sbin/$i
       echo -n " $i"
     done
     echo "."
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
