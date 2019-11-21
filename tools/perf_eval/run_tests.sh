#!/bin/bash

if [ $1 != "" ]; then
    tval=$1
else
    tval=60
fi

bytes=$(($tval * 1000000000/8))


kset="100 200 500 1000 2000 5000 10000 20000 50000"
tset="64 128 256 512 1024 1280"

for k in $kset ; do
  for t in $tset ; do
    for I in kval kval1.05 ; do
      case $I in
      kval)
        args="-I ${k}"
        ;;
      kval20)
	if [ $k -ge 1000 ]; then
	Ival=$(($k + 20))  
        args="-I ${Ival}"
	else
	    continue
	fi
        ;;
      kval1.05)
	Ival=$(($k * 105 / 100))  
        args="-I ${Ival}"
        ;;
      esac
      blksize=$(($k * $t))
      v=$(($bytes / $blksize))
      echo "v = $v"
      nSrcBlk=$((($v + 10000 - 1)/10000))
      nIter=$((($v + $nSrcBlk - 1)/$nSrcBlk))
      echo "../build/test_perf -K $k -T $t -l 100 -O $k -s -1 -n $nSrcBlk -c $nIter $args "
    done
  done
done
