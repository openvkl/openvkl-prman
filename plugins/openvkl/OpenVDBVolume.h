// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <openvdb/openvdb.h>
#include <openvkl/openvkl.h>
#include <rkcommon/math/box.h>
#include <rkcommon/math/vec.h>

using namespace rkcommon::math;

// loads OpenVDB volumes and converts them to Open VKL volumes
struct OpenVDBVolume
{
  OpenVDBVolume(const std::string &filename, const std::string &varName);
  virtual ~OpenVDBVolume() {}

  VKLVolume toVKLVolumeStructuredRegular();
  VKLVolume toVKLVolumeVDB();
  VKLVolume toVKLVolumeAuto();

  vec3f getVoxelSize();

 private:
  openvdb::GridBase::Ptr baseGrid;
};
