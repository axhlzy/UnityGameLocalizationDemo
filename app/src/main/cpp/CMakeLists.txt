# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(DobbyHome D:/app/DobbyTest/app/src/main/cpp/Dobby)
include_directories(
        ${DobbyHome}/include
        ${DobbyHome}/builtin-plugin
        ${DobbyHome}/builtin-plugin/SymbolResolver
        ${DobbyHome}/builtin-plugin/AndroidRestriction
        ${DobbyHome}/builtin-plugin/ApplicationEventMonitor
        ${DobbyHome}/external/logging
        ${DobbyHome}/builtin-plugin/SymbolResolver/macho
)

add_library(
        inject

        SHARED

#        ${DobbyHome}/builtin-plugin/AndroidRestriction/android_restriction_demo.cc

#        ${DobbyHome}/builtin-plugin/ApplicationEventMonitor/posix_file_descriptor_operation_monitor.cc
#        ${DobbyHome}/builtin-plugin/ApplicationEventMonitor/dynamic_loader_monitor.cc

        inject.cpp)

find_library(
        log-lib

        log)


target_link_libraries(
        inject

        dobby

        ${log-lib})

macro(SET_OPTION option value)
    set(${option} ${value} CACHE INTERNAL "" FORCE)
endmacro()
SET_OPTION(DOBBY_DEBUG ON)
SET_OPTION(DOBBY_GENERATE_SHARED OFF)
SET_OPTION(DynamicBinaryInstrument ON)
SET_OPTION(NearBranch ON)
SET_OPTION(Plugin.SymbolResolver ON)
add_subdirectory(${DobbyHome} dobby)