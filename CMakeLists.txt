## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## Global CMake options ##

cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(prman-vkl VERSION 0.7.0 LANGUAGES C CXX)


set(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})


if (NOT DEFINED RMANTREE)
  set(RMANTREE $ENV{RMANTREE})
  if (NOT DEFINED RMANTREE)
    message(FATAL_ERROR "Please set RMANTREE to point to your Renderman prefix")
  endif()
endif()
message(STATUS "Using RMANTREE=${RMANTREE}")

# OpenVDB comes with a set of cmake scripts that help find its dependencies.
# We must set the module path to find these scripts.
list(APPEND CMAKE_MODULE_PATH ${OpenVDB_ROOT}/lib/cmake/OpenVDB ${OpenVDB_ROOT}/lib64/cmake/OpenVDB)
find_package(OpenVDB 8.0.0 COMPONENTS openvdb)
if (NOT OpenVDB_FOUND)
  message(FATAL_ERROR "Failed to find OpenVDB at OpenVDB_ROOT (${OpenVDB_ROOT}).")
endif()

# Add all plugins.
add_subdirectory(plugins)
