## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

#######################
# Benchmark RIF filter.
#######################

add_library(rif_benchmark
  SHARED
    rif_benchmark.cpp)

target_include_directories(rif_benchmark
  PRIVATE
    ${RMANTREE}/include)
