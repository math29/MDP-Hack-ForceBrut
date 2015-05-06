#!/bin/sh

NB_CPU=`egrep '^processor' /proc/cpuinfo | wc -l`

if [ "${NB_CPU}" -lt "1" ] ; then
  NB_CPU=1
fi

killall "$1" >/dev/null 2>&1
for i in `seq ${NB_CPU}` ; do
  nice "$@" &
done

