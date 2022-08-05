#!/bin/bash -xe
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

ROOT_DIR=`pwd`

source gitlab/build_env.sh
source gitlab/prman.sh

################################################################################
# scene rendering                                                              #
################################################################################

# SCENE_DATA_ROOT env var defined in CI configuration

cd scenes

SCENE_NAME="smoke"
rsync -arP ${SCENE_DATA_ROOT}/${SCENE_NAME}/* ./
prman_openvdb ${SCENE_NAME}
prman_openvkl_structuredRegular ${SCENE_NAME}
prman_openvkl_vdb ${SCENE_NAME}

SCENE_NAME="wdas_cloud"
rsync -arP ${SCENE_DATA_ROOT}/${SCENE_NAME}/* ./
prman_openvdb ${SCENE_NAME}
prman_openvkl_structuredRegular ${SCENE_NAME}
prman_openvkl_vdb ${SCENE_NAME}

################################################################################
# gather results                                                               #
################################################################################

mkdir ${ROOT_DIR}/results
mv *.tif ${ROOT_DIR}/results/
mv *.xml ${ROOT_DIR}/results/
