#!/bin/sh
#
# decnet.sh
#
# Starts/Stops DECnet processes
#
# This script should go in /etc/rc.d/init.d
#
# and you should link to it from the relevant runlevel startup directory
# eg:
#     (RedHat)
#      ln -s /etc/rc.d/init.d/decnet.sh /etc/rc.d/rc3.d/S09decnet
#
#     (Caldera)
#      ln -s /etc/rc.d/init.d/decnet.sh /etc/rc.d/rc5.d/S01decnet
#
# This script MUST be run before TCP/IP is started unless you have a DEC
# TULIP based ethernet card AND are running Linux 2.2
#
# -----------------------------------------------------------------------------
#
# Daemons to start. You may remove the ones you don't want
#
prefix=/usr/local
daemons="dnetd phoned"

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
# See which distribution we are using and customise the start/stop 
# commands and the console display.
#
if [ -d /var/lib/dpkg ]
then
  # Debian
  startcmd="start-stop-daemon --start --quiet --exec"
  stopcmd="start-stop-daemon --stop --quiet --exec"
  startecho="\$i"
  startendecho="."
  stopendecho="done."
elif [ -d /var/lib/YaST ]
then
  # SuSE
  . /etc/rc.config
  startcmd=""
  stopcmd="killproc -TERM"
  startendecho=""
  stopendecho="done."
else
  # Assume RedHat
  . /etc/rc.d/init.d/functions
  startcmd="daemon"
  stopcmd="killproc"
  startendecho=""
  stopendecho="done."
fi

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
       if [ $? != 0 ]
       then
         echo error starting socket layer.
         exit 1
       fi
       NODE=`grep executor /etc/decnet.conf|sed 's/.* \([0-9.]\{3,7\}\) .*/\1/'`
       echo "$NODE" > /proc/sys/net/decnet/node_address
       CCT=`grep executor /etc/decnet.conf|sed 's/.*line *//'`
       echo "$CCT" > /proc/sys/net/decnet/default_device
     fi

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
