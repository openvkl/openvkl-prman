// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "OpenVKLImplicitField.h"

class OpenVKLVertexValue : public ImplicitVertexValue
{
 public:
  OpenVKLVertexValue(OpenVKLImplicitField *openvklImplicitField,
                     OpenVKLStats *stats,
                     VKLSampler sampler)
      : openvklImplicitField(openvklImplicitField),
        stats(stats),
        sampler(sampler)
  {
  }

  virtual ~OpenVKLVertexValue();

 protected:
  OpenVKLImplicitField *openvklImplicitField{nullptr};
  OpenVKLStats *stats{nullptr};
  VKLSampler sampler{nullptr};
};

class OpenVKLVertexValueFloat : public OpenVKLVertexValue
{
 public:
  OpenVKLVertexValueFloat(OpenVKLImplicitField *openvklImplicitField,
                          OpenVKLStats *stats,
                          VKLSampler sampler,
                          ValueMap valueMap)
      : OpenVKLVertexValue(openvklImplicitField, stats, sampler),
        valueMap(valueMap)
  {
  }

  void GetVertexValue(RtFloat *result, const RtPoint p) override;

  void GetVertexValueMultiple(int neval,
                              RtFloat *result,
                              int resultstride,
                              const RtPoint *p) override;

 private:
  ValueMap valueMap;
};

class OpenVKLVertexValueVec3f : public OpenVKLVertexValue
{
 public:
  OpenVKLVertexValueVec3f(OpenVKLImplicitField *openvklImplicitField,
                          OpenVKLStats *stats,
                          VKLSampler sampler)
      : OpenVKLVertexValue(openvklImplicitField, stats, sampler)
  {
  }

  void GetVertexValue(RtFloat *result, const RtPoint p) override;

  void GetVertexValueMultiple(int neval,
                              RtFloat *result,
                              int resultstride,
                              const RtPoint *p) override;
};
