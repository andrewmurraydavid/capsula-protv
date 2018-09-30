#!/bin/bash

 sudo kill -9 $(ps aux | grep 'capsula.local' | awk '{print $2}')
