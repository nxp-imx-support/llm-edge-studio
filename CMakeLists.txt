# Copyright 2024-2026 NXP
#
# NXP Proprietary. This software is owned or controlled by NXP and may only be
# used strictly in accordance with the applicable license terms.  By expressly
# accepting such terms or by downloading, installing, activating and/or
# otherwise using the software, you are agreeing that you have read, and that
# you agree to comply with and are bound by, such license terms.  If you do
# not agree to be bound by the applicable license terms, then you may not
# retain, install, activate or otherwise use the software.

cmake_minimum_required(VERSION 3.28)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Define version info
set(APP_VERSION_MAJOR 2)
set(APP_VERSION_MINOR 0)
set(APP_VERSION_PATCH 0)

# Configure version header
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/inc/version.h
  @ONLY
)

project(
  LLM-Edge-Studio
  VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}
  LANGUAGES CXX
)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable automatic Qt tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES)

# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.h)

# Define QML resources
set(QML_RESOURCES qml/resources.qrc)

# Create executable target
qt_add_executable(llm_edge_studio 
  ${SOURCES} 
  ${HEADERS} 
  ${QML_RESOURCES}
)
# Target-specific compile definitions
target_compile_definitions(llm_edge_studio 
  PRIVATE 
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
    $<$<CONFIG:Release>:QT_NO_INFO_OUTPUT>
)
# Target-specific compile options using generator expressions
target_compile_options(llm_edge_studio 
  PRIVATE
    -Wall
    $<$<CONFIG:Debug>:-O0 -g -fno-inline>
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g>
    $<$<CONFIG:MinSizeRel>:-Os>
)

# Include directories
target_include_directories(llm_edge_studio
  PRIVATE
    include
    ${CMAKE_CURRENT_BINARY_DIR}/inc
)

# Link libraries
target_link_libraries(llm_edge_studio
  PRIVATE
    Qt6::Quick
)

# Display build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Project version: ${PROJECT_VERSION}")
