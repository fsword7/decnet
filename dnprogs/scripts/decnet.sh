#!/bin/sh
#
# decnet.sh
#
# Starts/Stops DECnet processes
#
# chkconfig: - 09 91
# description:  DECnet.
# processname: dnetd
# config: /etc/decnet.conf
#
#
# This script should go in
#  /etc/init.d for redhat 7.0 onwards
#  /etc/rc.d/init.d for redhat up to 6.2
#
# You can install it using the following command:
#
# chkconfig --level 345 decnet on
#
# -----------------------------------------------------------------------------
#
# Daemons to start. You may remove the ones you don't want
#
daemons="dnetd phoned"

# Prefix for where the progs are installed. This is where the RPM puts them
prefix=/usr

#
# Interfaces to set the MAC address of. If empty all available
# ethernet interfaces will have their MAC address set the the DECnet
# address. If you do not want to do that (or don't want to do it here)
# then remove the -hw switch from the command.
#
# If running on Caldera OpenLinux you may need to add the -f switch to
# startnet to force it to change the MAC address because that
# distribution's startup scripts UP all the interfaces before calling any
# other scripts :-(
#
interfaces=""

startnet="$prefix/sbin/startnet -hw $interfaces"

#
# Set up some variables.
#
. /etc/rc.d/init.d/functions
startcmd="daemon"
stopcmd="killproc"
startendecho=""
stopendecho="done."

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

     NODE=`grep executor /etc/decnet.conf| awk '{print $2}'`
     echo "$NODE" > /proc/sys/net/decnet/node_address
     CCT=`grep executor /etc/decnet.conf | awk '{print $6}'`
     echo "$CCT" > /proc/sys/net/decnet/default_device
     $prefix/sbin/setether $NODE $CCT

     for i in $daemons
     do
       $startcmd $prefix/sbin/$i
       echo -n " `eval echo $startecho`"
     done
     echo "$startendecho"
     ;;

   stop)
     echo -n "Stopping DECnet... "
     for i in $daemons
     do
       $stopcmd $prefix/sbin/$i
     done
     echo "$stopendecho"
     ;;

   restart|reload|force-reload)
     echo -n "Restarting DECnet: "
     for i in $daemons
     do
       $stopcmd $prefix/sbin/$i
       $startcmd $prefix/sbin/$i
       echo -n "$startecho"
     done
     echo "$stopendecho"
     ;;

   *)
     echo "Usage $0 {start|stop|restart|force-reload}"
     ;;
esac

exit 0
