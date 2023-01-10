Steps to run:
1. git **clone**
2. install **libraries** (`sudo apt-get install libgflags-dev qt5-default sox cmake`)
3. download **submodule** (`git submodule init && git submodule update`)
5. **Make** (`mkdir build  && cd build && cmake .. && make`)
6. **Config files and iflytek library** (cp libs/* to build/bin/  [iflytek.cfg存放讯飞开放平台应用id，msc模型文件，libmsc.so动态库])
7. manual **RUN** OR run rec_and_test.sh