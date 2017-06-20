#!bin/bash
dirName=/dev/input/
fileName=`grep "Elo MultiTouch(MT) Device Input Module" /proc/bus/input/devices -A 5|awk '{print $3}' | grep -e "^event"`
device=$dirName$fileName
echo $device
nohup ./Gesture -r $device 1>&2 2>/dev/null &

