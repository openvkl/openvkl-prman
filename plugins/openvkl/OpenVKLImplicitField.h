// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ImplicitField.h>
#include <RixInterfaces.h>
#include <openvkl/openvkl.h>
#include <vector>

#include <rkcommon/math/box.h>
using namespace rkcommon::math;

#include <openvkl/utility/vdb/InnerNodes.h>
using namespace openvkl::utility::vdb;

struct OpenVKLStats;
struct OpenVDBVolume;

struct ValueMap
{
  ValueMap() {}
  ValueMap(float scale, float rolloff) : scale(scale), rolloff(rolloff) {}

  operator bool() const
  {
    return scale != 1.f || rolloff != 0.f;
  }

  float map(float value) const
  {
    if (rolloff > 0.f) {
      float valueNorm = value / rolloff;
      if (valueNorm > 1.f) {
        value = rolloff * (1.f + std::log(valueNorm));
      }
    }

    return value * scale;
  }

 private:
  float scale   = 1.f;
  float rolloff = 0.f;
};

class OpenVKLImplicitField : public ImplicitField
{
 public:
  OpenVKLImplicitField(OpenVKLStats *stats,
                       const std::string &filename,
                       const std::string &vklVolumeType,
                       const std::string &varName,
                       const ValueMap &densityMap);

  virtual ~OpenVKLImplicitField();

  float MinimumVoxelSize(const RtPoint corners[8]) override;

  void Range(RtInterval result,
             const RtPoint corners[8],
             const RtVolumeHandle h) override;

  RtFloat Eval(const RtPoint p) override;

  void EvalMultiple(int neval,
                    float *result,
                    int resultstride,
                    const RtPoint *p) override;

  void GradientEval(RtPoint result, const RtPoint p) override;

  void GradientEvalMultiple(int neval,
                            RtPoint *result,
                            const RtPoint *p) override;

  ImplicitVertexValue *CreateVertexValue(const RtUString name,
                                         int nvalue) override;

  static VKLDevice globalVKLDevice;

 private:
  const std::string filename;
  const std::string vklVolumeType;
  const std::string varName;

  ValueMap densityMap;

  VKLVolume volume{nullptr};
  VKLSampler sampler{nullptr};

  vec3f voxelSize;

  std::vector<InnerNode> innerNodes;

  OpenVKLStats *stats{nullptr};
};
