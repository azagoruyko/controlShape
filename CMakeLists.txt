cmake_minimum_required(VERSION 3.0)

project (controlShape) 

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_compile_options(/W0)

find_package( Maya REQUIRED )

set(source
    source/controlShape.cpp 
    source/controlShape.h
    source/main.cpp)

add_library(controlShape SHARED ${source})

MAYA_PLUGIN( controlShape )
