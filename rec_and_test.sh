#!/bin/bash
cd ./build/bin
./kws_iflytek_test &
rec -r 16k -b 16 -c 1 -e signed-integer -| nc -N localhost 7779
cd ../..