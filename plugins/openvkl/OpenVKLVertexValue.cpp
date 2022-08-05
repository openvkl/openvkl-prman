// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OpenVKLVertexValue.h"
#include "OpenVKLStats.h"

OpenVKLVertexValue::~OpenVKLVertexValue()
{
  if (sampler) {
    vklRelease(sampler);
  }
}

void OpenVKLVertexValueFloat::GetVertexValue(RtFloat *result, const RtPoint p)
{
#ifdef OPENVKL_ENABLE_STATS
  if (stats) {
    stats->accumulateEvals(1);
  }
#endif

  *result = vklComputeSample(sampler, (vkl_vec3f *)p);

  if (valueMap) {
    *result = valueMap.map(*result);
  }
}

void OpenVKLVertexValueFloat::GetVertexValueMultiple(int neval,
                                                     RtFloat *result,
                                                     int resultstride,
                                                     const RtPoint *p)
{
#ifdef OPENVKL_ENABLE_STATS
  if (stats) {
    stats->accumulateEvals(neval);
  }
#endif

  if (resultstride != 1) {
    for (int i = 0; i < neval; ++i) {
      *result = vklComputeSample(sampler, (const vkl_vec3f *)p);

      if (valueMap) {
        *result = valueMap.map(*result);
      }

      result += resultstride;
      ++p;
    }
  } else {
    vklComputeSampleN(sampler, neval, (const vkl_vec3f *)p, result);

    if (valueMap) {
      for (int i = 0; i < neval; ++i) {
        result[i] = valueMap.map(result[i]);
      }
    }
  }
}

void OpenVKLVertexValueVec3f::GetVertexValue(RtFloat *result, const RtPoint p)
{
#ifdef OPENVKL_ENABLE_STATS
  if (stats) {
    stats->accumulateEvals(1);
  }
#endif

  constexpr unsigned int attributeIndices[] = {0, 1, 2};

  vklComputeSampleM(
      sampler, (const vkl_vec3f *)p, (float *)result, 3, attributeIndices);
}

void OpenVKLVertexValueVec3f::GetVertexValueMultiple(int neval,
                                                     RtFloat *result,
                                                     int resultstride,
                                                     const RtPoint *p)
{
#ifdef OPENVKL_ENABLE_STATS
  if (stats) {
    stats->accumulateEvals(neval);
  }
#endif

  constexpr unsigned int attributeIndices[] = {0, 1, 2};

  if (resultstride != 3) {
    for (int i = 0; i < neval; ++i) {
      vklComputeSampleM(
          sampler, (const vkl_vec3f *)p, (float *)result, 3, attributeIndices);
      result += resultstride;
      ++p;
    }
  } else {
    vklComputeSampleMN(sampler,
                       neval,
                       (const vkl_vec3f *)p,
                       (float *)result,
                       3,
                       attributeIndices);
  }
}
