#!/bin/bash -xe
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# RenderMan environment
source /opt/pixar/env.sh

# dependencies
DEP_INSTALL_DIR=`pwd`/openvkl/build/install
export LD_LIBRARY_PATH=$DEP_INSTALL_DIR/lib:${LD_LIBRARY_PATH}

export openvkl_DIR=${DEP_INSTALL_DIR}
export RKCOMMON_TBB_ROOT=$DEP_INSTALL_DIR
export rkcommon_DIR=$DEP_INSTALL_DIR
export embree_DIR=$DEP_INSTALL_DIR
export IlmBase_ROOT=$DEP_INSTALL_DIR
export TBB_ROOT=$DEP_INSTALL_DIR
export Blosc_ROOT=$DEP_INSTALL_DIR

export OPENVKL_EXTRA_OPENVDB_OPTIONS="-DCMAKE_NO_SYSTEM_FROM_IMPORTED=ON"

# OpenVDB needs a newer CMake
export PATH=/home/visuser/local/cmake-3.19.4-Linux-x86_64/bin:${PATH}

mkdir build
cd build

cmake -L \
  -D OpenVDB_ROOT=$DEP_INSTALL_DIR ${OPENVKL_EXTRA_OPENVDB_OPTIONS} \
  -D ZLIB_ROOT=$DEP_INSTALL_DIR \
  -D BOOST_ROOT=$DEP_INSTALL_DIR \
  $@ \
  ..

make -j `nproc`
