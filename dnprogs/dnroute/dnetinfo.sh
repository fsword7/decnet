#!/bin/sh
#
# Quick & dirty shell script to query dnetd for the 
# node routing status.
#
# If that's not available then show the current neighbour table.
#

# These bizarre awk-isms are to cope with different versions of awk, sigh

show_all()
{
  awk '{ if (ARGIND == 1 || FILENAME == "/etc/decnet.conf") node[$2] = $4;\
       if (ARGIND == 2 || FILENAME == "/proc/net/decnet_neigh") printf "%-10s %-12s %s\n", $1, node[$1], $6}' \
          /etc/decnet.conf /proc/net/decnet_neigh | sort -g
}

if [ "$1" = "-l" ]
then
  show_all
  exit 0
fi

killall -USR1 dnroute 2>/dev/null
if [ "$?" = "0" ]
then
  cat /var/run/dnroute.status
else
  show_all
fi
