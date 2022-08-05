#!/bin/bash -x
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e
KW_SERVER_PATH=$KW_PATH/server
KW_CLIENT_PATH=$KW_PATH/client
export KLOCWORK_LTOKEN=/tmp/ltoken
echo "$KW_SERVER_IP;$KW_SERVER_PORT;$KW_USER;$KW_LTOKEN" > $KLOCWORK_LTOKEN
mkdir -p $CI_PROJECT_DIR/klocwork
log_file=$CI_PROJECT_DIR/klocwork/build.log

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

# build
$KW_CLIENT_PATH/bin/kwinject make -j `nproc` | tee -a $log_file
$KW_SERVER_PATH/bin/kwbuildproject -v --classic --url http://$KW_SERVER_IP:$KW_SERVER_PORT/$KW_PROJECT_NAME --tables-directory $CI_PROJECT_DIR/kw_tables kwinject.out | tee -a $log_file
$KW_SERVER_PATH/bin/kwadmin --url http://$KW_SERVER_IP:$KW_SERVER_PORT/ load --force --name build-$CI_JOB_ID $KW_PROJECT_NAME $CI_PROJECT_DIR/kw_tables | tee -a $log_file

# Store kw build name for check status later
echo "build-$CI_JOB_ID" > $CI_PROJECT_DIR/klocwork/build_name
