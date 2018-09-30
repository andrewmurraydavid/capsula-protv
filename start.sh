#!/bin/bash

cd /home/pi/ct/
/usr/bin/sudo /home/pi/ct/main  --led-rows=32 --led-cols=512 --led-gpio-mapping=adafruit-hat --led-slowdown-gpio=2
