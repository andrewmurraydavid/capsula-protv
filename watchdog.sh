#!/bin/bash

echo "Started single watcher"

file="/home/pi/ct/timeFile.txt"

LTIME=`stat -c %Z ${file}`

sleep 10;

NTIME=`stat -c %Z ${file}`

if [ "$LTIME" -ne "$NTIME" ]; then
    echo "Capsula running" >> /home/pi/ct/logs/watchdog
    echo "Capsula running"
    exit 0;
else
    echo "Capsula not running" >> /home/pi/ct/logs/watchdog
    echo "Capsula not running"
    sudo service capsula restart
fi
