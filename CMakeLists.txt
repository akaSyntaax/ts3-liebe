cmake_minimum_required(VERSION 3.16)
project(Liebe)

set(CMAKE_CXX_STANDARD 14)

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(defLibSuffix "_win64.dll")
    else ()
        set(defLibSuffix "_win32.dll")
    endif ()
elseif (APPLE)
    set(defLibSuffix "_mac.so")
else ()
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(defLibSuffix "_linux_amd64.so")
    else ()
        set(defLibSuffix "_linux_x86.so")
    endif ()
endif ()

set(libSuffix ${defLibSuffix} CACHE STRING "output library suffix")

add_library(Liebe SHARED src/plugin.c src/plugin.h)

target_include_directories(Liebe PUBLIC "include")
set_target_properties(Liebe PROPERTIES
        SUFFIX ${libSuffix}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE release
        )
