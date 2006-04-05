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
# Interfaces to set the MAC address of are specified in /etc/default/decnet
# The variable DNET_INTERFACES should be either set to a list of interfaces
# or "all". If it is empty then no interfaces will be modified.
#
# The MAC address *must* be set for DECnet to work so if you do not use this
# program you must do it some other way.
#
# ROUTING specifies whether we should be a endnode (0), level 1 router (1)
# or aread router (2)
#
# PRIORITY specifies the routing priority. Defaults to 32. Note VMS defaults
# to 64, max is 127.

[ ! -f /sbin/setether ] && exit 0


. /etc/default/decnet

interfaces="$DNET_INTERFACES"

ADDR="`grep executor /etc/decnet.conf | cut -f2`"

setether="/sbin/setether $ADDR $interfaces"


set_routing()
{

# Enable routing if required
if [ -n "$ROUTING" ]
then

# Set a default priority lower than VMS
    if [ -z "$PRIORITY" ]
    then
	PRIORITY=32
    fi

    for i in /proc/sys/net/decnet/conf/eth[0-9]*
    do
      echo "$1"        > $i/forwarding
      echo "$PRIORITY" > $i/priority
    done 
fi

}



case $1 in
   start)
     if [ ! -f /etc/decnet.conf ]
     then
       echo "DECnet not started as it is not configured."
       exit 0
     fi

     # If there is no DECnet in the kernel then try to load it.
     if [ ! -f /proc/net/decnet ]
     then
       modprobe decnet
       if [ ! -f /proc/net/decnet ]
       then
         echo "DECnet not started as it is not in the kernel."
	 exit 0
       fi
     fi

     echo -n "Starting DECnet..."
     $setether
     echo "$ADDR" > /proc/sys/net/decnet/node_address
     set_routing $ROUTING
     echo "done."
     ;;

   stop)
     set_routing 0
     ;;

   restart|reload|force-reload)
     ;;

   *)
     echo "Usage $0 {start|stop|restart|reload|force-reload}"
     ;;
esac

exit 0
