#!/bin/bash
#
# Set the MAC address of any/all ethernet cards
# for DECnet
#
# Parameters:
#
# $1 - DECnet address in the usual area.node format
# $2 - optional list of ethernet card names to set.
#

calc_ether()
{
  MACADDR=""

  ADDR=`echo $1 | sed -n 's/\([0-9]*\.[0-9]*\)/\1/p'`
  AREA=`echo $ADDR | sed -n 's/\([0-9]*\)\.\([0-9]*\)/\1/p'`
  NODE=`echo $ADDR | sed -n 's/\([0-9]*\)\.\([0-9]*\)/\2/p'`

  [ -z "$AREA" ] && AREA=0
  [ -z "$NODE" ] && NODE=0

  if [ "$NODE" -le 1023 -a  "$NODE" -ge 1 -a "$AREA" -le 63 -a "$AREA" -ge 1 ]
  then
    NUM=$(($AREA*1024 + $NODE))
    MACADDR="`printf \"AA:00:04:00:%02X:%02X\" $((NUM%256)) $((NUM/256))`"
  else
    exit 1
  fi
  return 0
}

calc_ether $1
if [ $? != 0 ]
then
  exit 1
fi

CARDS=$2
if [ -z "$CARDS" ]
then
  exit 0
fi

if [ "$CARDS" = "all" -o "$CARDS" = "ALL" ]
then
  CARDS=`cat /proc/net/dev|grep eth|cut -f1 -d':'`
fi

for i in $CARDS
do
  ifconfig $i hw ether $MACADDR up
done