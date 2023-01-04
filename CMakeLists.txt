cmake_minimum_required(3.19)

project(cpr_kws_iflytek_test)
set(target_bin kws_iflytek_test）
message("start build project " ${PROJECT_NAME})

EXECUTABLE_OUTPUT_PATH(${PROJECT_SOURCE_DIR}/bin)
PROJECT_BINARY_DIR(${PROJECT_SOURCE_DIR}/build)

set(glog ${PROJECT_SOURCE_DIR}/third_party/glog)

add_subdirectory(glog ${PROJECT_SOURCE_DIR}/third_party/glog )

include_directories(./include)
set(SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cc)

add_binary(${target_bin} ${SRC_FILES})
target_link_libraries (${target_bin} glog)
