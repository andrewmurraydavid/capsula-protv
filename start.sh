#!/bin/bash

cd /home/pi/ct/
/usr/bin/sudo nice -n -20 /home/pi/ct/main  --led-rows=32 --led-cols=512 --led-gpio-mapping=adafruit-hat --led-slowdown-gpio=2 --led-pwm-bits=1 --led-scan-mode=1
