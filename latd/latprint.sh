#!/bin/sh
#
# latprint.sh
#
# This is an example script for using latd as a LAT printer server
#
# on Linux:
#   latcp -A -a PRINTER -C '/bin/sh /usr/local/sbin/latprint.sh' -u root
#
# on VMS:
#   mcr latcp create port lta999
#   mcr latcp set port lta999/noqueue/service=PRINTER
#   init/queue/process=latsym/on=lta999:/start LINUXLAT
#

trap "" 1
stty -echo

cat > /tmp/latprint$$
lpr -r -oraw /tmp/latprint$$ 

