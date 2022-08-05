## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

##################
# Open VKL plugin
##################

option(BUILD_OPENVKL_PLUGIN "Build Open VKL plugin." ON)
option(OPENVKL_ENABLE_STATS "Enable plugin stats reporting." OFF)

if (BUILD_OPENVKL_PLUGIN)
  find_package(openvkl 1.3.0 REQUIRED)

  add_library(prman_openvkl
    SHARED
      OpenVKLImplicitField.cpp
      OpenVKLVertexValue.cpp
      OpenVDBVolume.cpp)

  if (OPENVKL_ENABLE_STATS)
    target_compile_definitions(prman_openvkl PRIVATE "OPENVKL_ENABLE_STATS")
  endif()

  target_link_libraries(prman_openvkl
    PUBLIC
      openvkl::openvkl
      OpenVDB::openvdb)

  target_include_directories(prman_openvkl
    PRIVATE
      ${RMANTREE}/include)

  set(OPENVKL_DEVICES "cpu;cpu_4;cpu_8;cpu_16")

  if(NOT OPENVKL_DEVICE)
    set(OPENVKL_DEVICE "cpu" CACHE STRING "Choose the Open VKL device to use." FORCE)
  endif()
  set_property(CACHE OPENVKL_DEVICE PROPERTY STRINGS ${OPENVKL_DEVICES})

  target_compile_definitions(prman_openvkl PRIVATE OPENVKL_DEVICE="${OPENVKL_DEVICE}")

endif()
