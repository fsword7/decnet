#!/bin/sh
#
# decnet.sh NO_RESTART_ON_UPGRADE
#
# Starts/Stops DECnet processes
#
# This script should go in /etc/init.d      (Debian)
#                          /etc/rc.d/init.d (RedHat)
#                          /sbin/init.d     (SuSE)
#
# and you should link to it from the relevant runlevel startup directory
# eg: (Debian)
#      ln -s /etc/init.d/decnet.sh /etc/rcS.d/S39decnet.sh
#
#     (RedHat)
#      ln -s /etc/rc.d/init.d/decnet.sh /etc/rc.d/rc3.d/S10decnet
#
#     (SuSE)
#      ln -s /sbin/init.d/decnet.sh /sbin/init.d/rc2.d/S05decnet
#
# This script MUST be run before TCP/IP is started unless you have a DEC
# TULIP based ethernet card.
#
# -----------------------------------------------------------------------------
#
# Daemons to start. You may remove the ones you don't want
#
prefix=/usr
daemons="fal ctermd dnmirror"

#
# the -hw flag to startnet tells it to set the hardware address of the ethernet
# card to match the DECnet node address. You probably don't need this if you
# are using the tulip or de4x5 drivers for your ethernet card. If you are
# unsure just leave it as it is.
#
startnet="$prefix/sbin/startnet -hw"

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
     echo -n "Starting DECnet: "

     $startnet
     if [ $? != 0 ]
     then
       echo error starting socket layer.
       exit 1
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

   restart|reload)
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
     echo "Usage $0 {start|stop|restart}"
     ;;
esac

exit 0
