#!/bin/sh
#
# decnet.sh 
#
# Sets up the ethernet interface(s).
#
# This script MUST be run before TCP/IP is started.
#
# ---------------------------------------------------------------------------
#
FLAGS="start 39 S .  stop 11 1 ."

#
# Interfaces to set the MAC address of are specified in /etec/default/decnet
# The variable DNET_INTERFACES should be either set to a list of interfaces
# or "all". If it is empty then no interfaces will be modified.
#
# The MAC address *must* be set for DECnet to work so if you do not use this
# program you must do it some other way.
#

[ ! -f /sbin/setether ] && exit 0

. /etc/default/decnet

interfaces="$DNET_INTERFACES"

ADDR="`grep executor /etc/decnet.conf | cut -f2`"

setether="/sbin/setether $ADDR $interfaces"

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
     $setether
     ;;

   stop)
     ;;

   restart|force-reload)
     ;;

   *)
     echo "Usage $0 {start|stop|restart|force-reload}"
     ;;
esac

exit 0
