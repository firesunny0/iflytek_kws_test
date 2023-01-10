#!/bin/bash
cd ./build/bin
./kws_iflytek_test &
rec -t raw -r 16k -b 16 -c 1 -e signed-integer -| nc localhost 7779
cd ../..