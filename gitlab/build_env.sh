#!/bin/bash
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# dependencies
DEP_INSTALL_DIR=`pwd`/openvkl/build/install
export LD_LIBRARY_PATH=$DEP_INSTALL_DIR/lib:${LD_LIBRARY_PATH}
