cmake_minimum_required(VERSION 3.0)

project (controlShape) 

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_compile_options(/W0)

find_package( Maya REQUIRED )

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_CRT_SECURE_NO_WARNINGS -D_MT -D_DLL -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_CRT_SECURE_NO_WARNINGS -D_MT -D_DLL /GL /Gy /Zi /sdl- /FC /Oi /permissive-")

set(source
    source/controlShape.cpp 
    source/controlShape.h
    source/main.cpp)

add_library(controlShape SHARED ${source})

target_include_directories(controlShape PUBLIC 
  ${MAYA_INCLUDE_DIR})

target_link_libraries(controlShape  
  ${MAYA_Foundation_LIBRARY} 
  ${MAYA_OpenMaya_LIBRARY} 
  ${MAYA_OpenMayaAnim_LIBRARY} 
  ${MAYA_OpenMayaUI_LIBRARY} 
  ${MAYA_OpenMayaRender_LIBRARY} 
  ${MAYA_tbb_LIBRARY}
  ${MAYA_clew_LIBRARY})

set_target_properties( controlShape PROPERTIES OUTPUT_NAME controlShape CLEAN_DIRECT_OUTPUT 1 SUFFIX ".mll" CXX_STANDARD 17)

MAYA_SET_PLUGIN_PROPERTIES( controlShape )

install(TARGETS controlShape RUNTIME DESTINATION .)

