#!/bin/sh
#
PARM=1
while [ $PARM ]
do
RET=`ps -ef | grep -c "Gesture -r"`
if [ $RET -eq 1 ]
then
        sh gestureSh.sh
fi
#echo "waite 30 seconds"
sleep 30
done
exit 0
