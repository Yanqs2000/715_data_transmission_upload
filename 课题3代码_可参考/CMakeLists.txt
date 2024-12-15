
cmake_minimum_required(VERSION 3.20)
project(main_auto_gcsu)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

find_package(glog REQUIRED)
find_package(pylon REQUIRED)
find_library(ZSTD_LIBRARY NAMES zstd)

file(GLOB_RECURSE src "src/*.cpp")

include_directories(/opt/pylon/include /home/pi/software_packge/zstd/lib)
include_directories(inc)

link_directories(/opt/pylon/lib /home/pi/software_packge/zstd/lib)
aux_source_directory(. DIR_SIR)

add_executable(${PROJECT_NAME} ${DIR_SIR} ${src})

# 设置可执行文件的输出目录（可选）CMAKE_BINARY_DIR:build目录，CMAKE_SOURCE_DIR: 1级目录
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}  pylon::pylon ${ZSTD_LIBRARY} glog::glog)

install( TARGETS ${PROJECT_NAME} )
