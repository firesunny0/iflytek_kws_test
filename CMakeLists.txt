cmake_minimum_required(VERSION 3.15.0)

project(cpr_kws_iflytek_test VERSION 0.1)

set(target_bin kws_iflytek_test)
message("start build project " ${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

aux_source_directory(${PROJECT_SOURCE_DIR}/include INCLUDE_FILES )


find_package(gflags REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)

link_directories(${PROJECT_SOURCE_DIR}/libs)


set(glog_DIR ${PROJECT_SOURCE_DIR}/third_party/glog)

add_subdirectory(${glog_DIR} ${LIBRARY_OUTPUT_PATH}/glog)

include_directories(${PROJECT_SOURCE_DIR}/include
	)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_FILES)

add_executable(${target_bin} ${SRC_FILES}  ${PROJECT_SOURCE_DIR}/include/cmd_asr_thread.h)
target_link_libraries (${target_bin}  glog::glog Qt5::Core msc)
