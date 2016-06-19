#!/bin/bash

BINARY=./ptfp_ba
DIR=logs_$(date +"%Y_%m_%d_%H%M%S")

mkdir $DIR
rm logs_current
ln -s $DIR logs_current
hostname > logs_current/host

for r in {1..10}; do
mkdir "logs_current/$r"
   for p in 16 14 12 10 8 6 4; do
      for n in \
          10m   17m   31m   56m \
          100m  170m  310m  560m \
          1000m 1700m 3100m 5600m \
          10g
      do
         PARAMS="-x $r $n 10"
         LOGFILE="logs_current/$r/ptfp_p${p}_n$n"

         echo "$PARAMS > $LOGFILE"

         if [ -f "$LOGFILE" ]
         then
            echo "Skip"
         else
            echo "Params: $PARAMS" > $LOGFILE
            $BINARY $PARAMS > $LOGFILE 2>&1
            grep "Time since the last reset" $LOGFILE |tail -n1
         fi
      done # n
   done # p
done # r