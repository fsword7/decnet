#!/bin/sh
#
# Check the kernel headers available to us. 
#
#
rm -f include/netdnet/dn.h

if [ -f /usr/src/linux/include/netdnet/dn.h ]
then
  #
  # Eduardo's kernel - only use dn.h if it doesn't define nodeent
  # (which belongs in dnetdb.h)
  #
  grep -q nodeent /usr/src/linux/include/netdnet/dn.h 
  if [ $? = 1 ]
  then
    echo Using dn.h from Eduardo\'s kernel
    cp  /usr/src/linux/include/netdnet/dn.h include/netdnet
  else
    echo Using dn.h from our distribution
    cp include/kernel/netdnet/dn.h include/netdnet
  fi
fi

if [ -f /usr/src/linux/include/linux/dn.h ]
then
  #
  # Steve's kernel
  #
  echo Using dn.h from Steve\'s kernel
  cp  /usr/src/linux/include/linux/dn.h include/netdnet
fi

if [ ! -f include/netdnet/dn.h ]
then
  #
  # Use our fallback include file
  #
  cp include/kernel/netdnet/dn.h include/netdnet
  echo '*********************************************************************'
  echo I can\'t find a patched kernel in /usr/src.
  echo
  echo If you haven\'t patched your kernel yet then I recommend you do
  echo so before compiling these programs because they certainly won\'t
  echo work without DECnet support in the kernel and it is important
  echo that the programs know which version of the kernel patch you
  echo are using.
  echo
  echo You can still compile the programs without the kernel available by
  echo typing the command \'make please\' but be aware that some of the
  echo programs may not work echo correctly or at all if you do this so make 
  echo sure you know what you are doing.
  echo
  return 1
fi
return 0