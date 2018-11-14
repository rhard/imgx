cmake_minimum_required(VERSION 3.12)
project(ImgX)

set(CMAKE_CXX_STANDARD 11)

if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
else()
    message(FATAL_ERROR "Please, use conan package manager or create own build script")
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()

add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1)

include_directories(imgui src)

FILE(GLOB HEADER_SRCS
        src/*.h
        test/*.h
        imgui/imconfig.h
        imgui/imgui.h
        imgui/imgui_internal.h
        )
FILE(GLOB CPP_SRCS
        src/*.cpp
        test/*.cpp
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        )

add_library(imgx_test SHARED ${CPP_SRCS} ${HEADER_SRCS} test/gui.cpp test/gui.h)
target_link_libraries(imgx_test CONAN_PKG::xplane_sdk opengl32)

set_target_properties(imgx_test PROPERTIES PREFIX "")
set_target_properties(imgx_test PROPERTIES OUTPUT_NAME "imgx_test")
set_target_properties(imgx_test PROPERTIES SUFFIX ".xpl")

ADD_CUSTOM_TARGET(deploy ALL
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:imgx_test> "Z:/X-Plane 11/Resources/plugins/imgx_test/win_x64"
        DEPENDS imgx_test)