#!/bin/bash
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# RenderMan environment
source /opt/pixar/env.sh

# prman common arguments
COMMON_ARGS="-loglevel 4 -rif ../build/plugins/rif_benchmark/librif_benchmark.so"

# determine processor IDs ('processor' field from /proc/cpuinfo) corresponding
# to the first two NUMA nodes and first cpu of each physical core; more simply,
# this gives us processor IDs for the first two CPU socket, as if hyperthreading
# were disabled.
NUMA_NODE_ID="0|1"
CPU_IDS=`lscpu -p=node,core,cpu | egrep ^${NUMA_NODE_ID}, | awk -F, '{print $2,$3}' | sort -n -u -t' ' -k1,1 | awk -F' ' '{print $2}' | paste -sd,`

NUM_CPU_IDS=`echo $CPU_IDS | tr ',' '\n'|wc -l`

echo "using NUMA nodes ${NUM_NODE_ID}, first cpu of each physical core:"
echo "  ${NUM_CPU_IDS} cpus: ${CPU_IDS}"

# for no NUMA restrictions
#NUMA_COMMAND_PREFIX=""

# NUMA settings: restricts to first cpu of each core on specified NUMA node(s)
# if using just one NUMA node, can also add: --membind=${NUMA_NODE_ID}
NUMA_COMMAND_PREFIX="numactl --physcpubind=${CPU_IDS}"
COMMON_ARGS="-t:${NUM_CPU_IDS} ${COMMON_ARGS}"

echo "NUMA_COMMAND_PREFIX: ${NUMA_COMMAND_PREFIX}"
echo "COMMON_ARGS: ${COMMON_ARGS}"

# constrain VKL to one thread only
export OPENVKL_THREADS=1

################################################################################
# prman render functions                                                       #
################################################################################

prman_openvdb () {
  if [ $# -eq 0 ]; then
    echo "no scene name provided"
    exit 1
  fi

  local SCENE_NAME=$1

  local RIF_ARGS="-rifargs -filterDisplay -plugin /opt/pixar/RenderManProServer-24.4/lib/plugins/impl_openvdb.so -rifend"
  local SUFFIX="openvdb"

  ${NUMA_COMMAND_PREFIX} prman ${COMMON_ARGS} \
    ${RIF_ARGS} \
    -statsfile ./${SCENE_NAME}_${SUFFIX}.xml \
    ${SCENE_NAME}.rib

  mv ${SUFFIX}.tif ${SCENE_NAME}_${SUFFIX}.tif
}

prman_openvkl_structuredRegular () {
  if [ $# -eq 0 ]; then
    echo "no scene name provided"
    exit 1
  fi

  local SCENE_NAME=$1

  local RIF_ARGS="-rifargs -filterDisplay -plugin ../build/plugins/openvkl/libprman_openvkl.so -type structuredRegular -rifend"
  local SUFFIX="openvkl_structuredRegular"

  ${NUMA_COMMAND_PREFIX} prman ${COMMON_ARGS} \
    ${RIF_ARGS} \
    -statsfile ./${SCENE_NAME}_${SUFFIX}.xml \
    ${SCENE_NAME}.rib

  mv ${SUFFIX}.tif ${SCENE_NAME}_${SUFFIX}.tif
}

prman_openvkl_vdb () {
  if [ $# -eq 0 ]; then
    echo "no scene name provided"
    exit 1
  fi

  local SCENE_NAME=$1

  local RIF_ARGS="-rifargs -filterDisplay -plugin ../build/plugins/openvkl/libprman_openvkl.so -type vdb -rifend"
  local SUFFIX="openvkl_vdb"

  ${NUMA_COMMAND_PREFIX} prman ${COMMON_ARGS} \
    ${RIF_ARGS} \
    -statsfile ./${SCENE_NAME}_${SUFFIX}.xml \
    ${SCENE_NAME}.rib

  mv ${SUFFIX}.tif ${SCENE_NAME}_${SUFFIX}.tif
}

prman_openvkl_auto () {
  if [ $# -eq 0 ]; then
    echo "no scene name provided"
    exit 1
  fi

  local SCENE_NAME=$1

  local RIF_ARGS="-rifargs -filterDisplay -plugin ../build/plugins/openvkl/libprman_openvkl.so -type auto -rifend"
  local SUFFIX="openvkl_auto"

  ${NUMA_COMMAND_PREFIX} prman ${COMMON_ARGS} \
    ${RIF_ARGS} \
    -statsfile ./${SCENE_NAME}_${SUFFIX}.xml \
    ${SCENE_NAME}.rib

  mv ${SUFFIX}.tif ${SCENE_NAME}_${SUFFIX}.tif
}
