// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OpenVDBVolume.h"
#include <openvdb/tools/GridTransformer.h>
#include <openvkl/utility/vdb/OpenVdbGrid.h>
#include <rkcommon/array3D/for_each.h>
#include <rkcommon/math/box.h>
#include <rkcommon/math/vec.h>
#include "OpenVKLImplicitField.h"

using namespace rkcommon::array3D;

OpenVDBVolume::OpenVDBVolume(const std::string &filename,
                             const std::string &varName)
{
  static bool openvdbInitialized = false;

  if (!openvdbInitialized) {
    openvdb::initialize();
    openvdbInitialized = true;
  }

  openvdb::io::File file(filename.c_str());

  file.open();

  for (openvdb::io::File::NameIterator nameIter = file.beginName();
       nameIter != file.endName();
       ++nameIter) {
    if (nameIter.gridName() == varName) {
      baseGrid = file.readGrid(nameIter.gridName());
    }
  }

  if (!baseGrid) {
    throw std::runtime_error("OpenVDBVolume: could not find grid. filename: " +
                             filename + ", varName: " + varName);
  }

  file.close();

  RixContext *ctx = RixGetContext();
  RixMessages *m  = (RixMessages *)ctx->GetRixInterface(k_RixMessages);

  auto _dims = baseGrid->evalActiveVoxelDim();

  m->InfoAlways(
      "OpenVDBVolume: filename: %s, varName: %s, dimensions: [%d, %d, %d]",
      filename.c_str(),
      varName.c_str(),
      _dims.x(),
      _dims.y(),
      _dims.z());
}

VKLVolume OpenVDBVolume::toVKLVolumeStructuredRegular()
{
  auto _dims             = baseGrid->evalActiveVoxelDim();
  const vec3i dimensions = vec3i(_dims.x() - 1, _dims.y() - 1, _dims.z() - 1);

  auto _bbox = baseGrid->evalActiveVoxelBoundingBox();
  const vec3i indexOrigin =
      vec3i(_bbox.min().x(), _bbox.min().y(), _bbox.min().z());

  VKLVolume vklVolumeStructured =
      vklNewVolume(OpenVKLImplicitField::globalVKLDevice, "structuredRegular");

  if (openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid)) {
    openvdb::FloatGrid::Ptr grid =
        openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

    std::vector<float> voxels;
    voxels.reserve(rkcommon::array3D::longProduct(dimensions));

    auto accessor = grid->getAccessor();

    for (size_t k = 0; k < dimensions.z; k++) {
      for (size_t j = 0; j < dimensions.y; j++) {
        for (size_t i = 0; i < dimensions.x; i++) {
          openvdb::Coord xyz(
              indexOrigin.x + i, indexOrigin.y + j, indexOrigin.z + k);
          float value = accessor.getValue(xyz);
          voxels.push_back(value);
        }
      }
    }

    if (rkcommon::array3D::longProduct(dimensions) != voxels.size()) {
      throw std::runtime_error(
          "inconsistent number of voxels in VKLVolume conversion");
    }

    VKLData data = vklNewData(OpenVKLImplicitField::globalVKLDevice,
                              voxels.size(),
                              VKL_FLOAT,
                              voxels.data());
    vklSetData(vklVolumeStructured, "data", data);
    vklRelease(data);

  } else if (openvdb::gridPtrCast<openvdb::Vec3fGrid>(baseGrid)) {
    openvdb::Vec3fGrid::Ptr grid =
        openvdb::gridPtrCast<openvdb::Vec3fGrid>(baseGrid);

    std::vector<rkcommon::math::vec3f> voxels;
    voxels.reserve(rkcommon::array3D::longProduct(dimensions));

    auto accessor = grid->getAccessor();

    for (size_t k = 0; k < dimensions.z; k++) {
      for (size_t j = 0; j < dimensions.y; j++) {
        for (size_t i = 0; i < dimensions.x; i++) {
          openvdb::Coord xyz(
              indexOrigin.x + i, indexOrigin.y + j, indexOrigin.z + k);
          openvdb::Vec3f value = accessor.getValue(xyz);
          voxels.push_back(vec3f(value.x(), value.y(), value.z()));
        }
      }
    }

    if (rkcommon::array3D::longProduct(dimensions) != voxels.size()) {
      throw std::runtime_error(
          "inconsistent number of voxels in VKLVolume conversion");
    }

    // we provide data to Open VKL in a strided layout; however we don't use
    // shared data arrays, so Open VKL will re-arrange the data; maintaining the
    // strided data layout (via using shared buffers) would likely improve
    // sampling performance.

    VKLData dataX = vklNewData(OpenVKLImplicitField::globalVKLDevice,
                               voxels.size(),
                               VKL_FLOAT,
                               &voxels[0].x,
                               VKL_DATA_DEFAULT,
                               sizeof(rkcommon::math::vec3f));

    VKLData dataY = vklNewData(OpenVKLImplicitField::globalVKLDevice,
                               voxels.size(),
                               VKL_FLOAT,
                               &voxels[0].y,
                               VKL_DATA_DEFAULT,
                               sizeof(rkcommon::math::vec3f));

    VKLData dataZ = vklNewData(OpenVKLImplicitField::globalVKLDevice,
                               voxels.size(),
                               VKL_FLOAT,
                               &voxels[0].z,
                               VKL_DATA_DEFAULT,
                               sizeof(rkcommon::math::vec3f));

    VKLData datas[] = {dataX, dataY, dataZ};

    VKLData dataCombined =
        vklNewData(OpenVKLImplicitField::globalVKLDevice, 3, VKL_DATA, datas);
    vklRelease(dataX);
    vklRelease(dataY);
    vklRelease(dataZ);

    vklSetData(vklVolumeStructured, "data", dataCombined);
    vklRelease(dataCombined);
  }

  else {
    throw std::runtime_error("unsupported OpenVDB grid type");
  }

  vklSetBool(vklVolumeStructured, "cellCentered", true);

  vklSetFloat(vklVolumeStructured, "background", 0.f);

  vklSetVec3i(vklVolumeStructured,
              "dimensions",
              dimensions.x,
              dimensions.y,
              dimensions.z);

  vklSetVec3i(vklVolumeStructured,
              "indexOrigin",
              indexOrigin.x,
              indexOrigin.y,
              indexOrigin.z);

  const auto &indexToObject = baseGrid->transform().baseMap();
  if (!indexToObject->isLinear())
    throw std::runtime_error(
        "OpenVKL only supports linearly transformed volumes");

  const auto &ri2o = indexToObject->getAffineMap()->getMat4();
  const auto *i2o  = ri2o.asPointer();
  AffineSpace3f openvdbIndexToObject;
  openvdbIndexToObject.l = LinearSpace3f(vec3f(i2o[0], i2o[1], i2o[2]),
                                         vec3f(i2o[4], i2o[5], i2o[6]),
                                         vec3f(i2o[8], i2o[9], i2o[10]));
  openvdbIndexToObject.p = vec3f(i2o[12], i2o[13], i2o[14]);

  vklSetParam(vklVolumeStructured,
              "indexToObject",
              VKL_AFFINE3F,
              &openvdbIndexToObject);

  vklCommit(vklVolumeStructured);

  return vklVolumeStructured;
}

VKLVolume OpenVDBVolume::toVKLVolumeVDB()
{
  const bool deferLeaves = false;
  const bool repackNodes =
      true;  // shared buffers not used when data is repacked

  if (openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid)) {
    openvdb::FloatGrid::Ptr grid =
        openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
    openvkl::utility::vdb::OpenVdbFloatGrid gridHelper(
        OpenVKLImplicitField::globalVKLDevice, grid, deferLeaves, repackNodes);

    VKLVolume volume = gridHelper.createVolume(false);

    vklSetFloat(volume, "background", 0.f);
    vklCommit(volume);

    return volume;

  } else if (openvdb::gridPtrCast<openvdb::Vec3fGrid>(baseGrid)) {
    openvdb::Vec3fGrid::Ptr grid =
        openvdb::gridPtrCast<openvdb::Vec3fGrid>(baseGrid);
    openvkl::utility::vdb::OpenVdbVec3sGrid gridHelper(
        OpenVKLImplicitField::globalVKLDevice, grid, deferLeaves, repackNodes);

    VKLVolume volume = gridHelper.createVolume(false);

    vklSetFloat(volume, "background", 0.f);
    vklCommit(volume);

    return volume;
  }

  else {
    throw std::runtime_error("unsupported OpenVDB grid type");
  }
}

VKLVolume OpenVDBVolume::toVKLVolumeAuto()
{
  RixContext *ctx = RixGetContext();
  RixMessages *m  = (RixMessages *)ctx->GetRixInterface(k_RixMessages);

  auto _dims             = baseGrid->evalActiveVoxelDim();
  const vec3i dimensions = vec3i(_dims.x() - 1, _dims.y() - 1, _dims.z() - 1);

  // determine which volume type to use based on memory overhead
  const size_t sparseMemUsage = baseGrid->memUsage();
  size_t denseMemUsage        = longProduct(dimensions);

  if (openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid)) {
    denseMemUsage *= sizeof(float);
  } else if (openvdb::gridPtrCast<openvdb::Vec3fGrid>(baseGrid)) {
    denseMemUsage *= (3 * sizeof(float));
  }

  const double denseMemoryOverheadFraction =
      double(denseMemUsage) / double(sparseMemUsage);

  const size_t denseMemoryOverheadBytes =
      denseMemUsage > sparseMemUsage ? denseMemUsage - sparseMemUsage : 0;

  if (denseMemoryOverheadFraction <= 1.5 ||
      denseMemoryOverheadBytes <= 50ll * 1024 * 1024) {
    m->InfoAlways("OpenVDBVolume: using VKL structuredRegular representation");
    return toVKLVolumeStructuredRegular();
  } else {
    m->InfoAlways("OpenVDBVolume: using VKL vdb representation");
    return toVKLVolumeVDB();
  }
}

vec3f OpenVDBVolume::getVoxelSize()
{
  auto vs = baseGrid->voxelSize();
  return vec3f(vs.x(), vs.y(), vs.z());
}
