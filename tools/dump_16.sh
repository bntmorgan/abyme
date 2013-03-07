#!/bin/sh
start=0
end=0
file=

if [ "$#" -lt 1 ];then
  echo "$0 file [ start-address stop-address ]"
  exit 1
fi

file=$1

if [ "$#" -gt 1 ];then
  start=$2
fi

if [ "$#" -lt 3 ];then
  end=`cat $1 | wc -c`
else
  end=$3
fi

echo $file $start $end

objdump -D -b binary -mi386 -Maddr16,data16 $file --start-address $start --stop-address $end
