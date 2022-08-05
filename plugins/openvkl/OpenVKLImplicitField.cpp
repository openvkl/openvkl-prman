// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OpenVKLImplicitField.h"
#include <memory>
#include <string>
#include <tuple>
#include "OpenVDBVolume.h"
#include "OpenVKLStats.h"
#include "OpenVKLVertexValue.h"
#include "external/json.hpp"

using json = nlohmann::json;

VKLDevice OpenVKLImplicitField::globalVKLDevice = nullptr;

// helper for instantiating VKLVolume's; returns a VKLVolume and the volume's
// voxel size
static std::tuple<VKLVolume, vec3f> getVKLVolume(
    const std::string &filename,
    const std::string &vklVolumeType,
    const std::string &varName)
{
  // we do not need to retain the OpenVDBVolume object; shared buffers are not
  // used
  auto openvdbVolume = std::make_unique<OpenVDBVolume>(filename, varName);

  VKLVolume volume = nullptr;

  if (vklVolumeType == "structuredRegular") {
    volume = openvdbVolume->toVKLVolumeStructuredRegular();
  } else if (vklVolumeType == "vdb") {
    volume = openvdbVolume->toVKLVolumeVDB();
  } else if (vklVolumeType == "auto") {
    volume = openvdbVolume->toVKLVolumeAuto();
  } else {
    throw std::runtime_error("OpenVKLImplicitField: unknown VKL volume type: " +
                             vklVolumeType);
  }

  return std::make_tuple(volume, openvdbVolume->getVoxelSize());
}

OpenVKLImplicitField::OpenVKLImplicitField(OpenVKLStats *stats,
                                           const std::string &filename,
                                           const std::string &vklVolumeType,
                                           const std::string &varName,
                                           const ValueMap &densityMap)
    : filename(filename),
      vklVolumeType(vklVolumeType),
      varName(varName),
      densityMap(densityMap),
      stats(stats)
{
  std::tie(volume, voxelSize) = getVKLVolume(filename, vklVolumeType, varName);

  sampler = vklNewSampler(volume);
  vklCommit(sampler);

  // populate bounding box in world space: [xMin, xMax, yMin, yMax, zMin, zMax]
  vkl_box3f vklBbox = vklGetBoundingBox(volume);

  bbox[0] = vklBbox.lower.x;
  bbox[1] = vklBbox.upper.x;

  bbox[2] = vklBbox.lower.y;
  bbox[3] = vklBbox.upper.y;

  bbox[4] = vklBbox.lower.z;
  bbox[5] = vklBbox.upper.z;

  innerNodes = getInnerNodes(volume, 2);
}

OpenVKLImplicitField::~OpenVKLImplicitField()
{
  if (volume) {
    vklRelease(volume);
  }

  if (sampler) {
    vklRelease(sampler);
  }
}

float OpenVKLImplicitField::MinimumVoxelSize(const RtPoint corners[8])
{
  return reduce_min(voxelSize);
}

void OpenVKLImplicitField::Range(RtInterval result,
                                 const RtPoint corners[8],
                                 const RtVolumeHandle h)
{
  PIXAR_ARGUSED(h);

  box3f rangeBox;
  for (int i = 0; i < 8; i++) {
    rangeBox.extend(vec3f(corners[i]));
  }

  range1f valueRange;

  for (const auto &node : innerNodes) {
    if (!disjoint(rangeBox, node.bbox)) {
      valueRange.extend(node.valueRange[0]);
    }
  }

  result[0] = valueRange.lower;
  result[1] = valueRange.upper;

  if (densityMap) {
    result[0] = densityMap.map(result[0]);
    result[1] = densityMap.map(result[1]);
  }
}

RtFloat OpenVKLImplicitField::Eval(const RtPoint p)
{
#ifdef OPENVKL_ENABLE_STATS
  if (stats) {
    stats->accumulateEvals(1);
  }
#endif

  float sample = vklComputeSample(sampler, (vkl_vec3f *)p);

  if (densityMap) {
    sample = densityMap.map(sample);
  }

  return sample;
}

void OpenVKLImplicitField::EvalMultiple(int neval,
                                        float *result,
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

      if (densityMap) {
        *result = densityMap.map(*result);
      }

      result += resultstride;
      ++p;
    }
  } else {
    vklComputeSampleN(sampler, neval, (const vkl_vec3f *)p, result);

    if (densityMap) {
      for (int i = 0; i < neval; ++i) {
        result[i] = densityMap.map(result[i]);
      }
    }
  }
}

void OpenVKLImplicitField::GradientEval(RtPoint result, const RtPoint p)
{
  vkl_vec3f grad = vklComputeGradient(sampler, (vkl_vec3f *)p);

  result[0] = grad.x;
  result[1] = grad.y;
  result[2] = grad.z;
}

void OpenVKLImplicitField::GradientEvalMultiple(int neval,
                                                RtPoint *result,
                                                const RtPoint *p)
{
  vklComputeGradientN(
      sampler, neval, (const vkl_vec3f *)p, (vkl_vec3f *)result);
}

ImplicitVertexValue *OpenVKLImplicitField::CreateVertexValue(
    const RtUString name, int nvalue)
{
  PIXAR_ARGUSED(nvalue);

  std::string fullName(name.CStr());
  size_t pos = fullName.find(" ");

  if (pos != std::string::npos) {
    std::string type             = fullName.substr(0, pos);
    std::string requestedVarName = fullName.substr(pos + 1);

    if (type == "float") {
      if (requestedVarName == varName) {
        VKLSampler newSampler = vklNewSampler(volume);
        vklCommit(newSampler);

        return new OpenVKLVertexValueFloat(this, stats, newSampler, densityMap);
      } else {
        // requesting a variable other than the field variable
        try {
          VKLVolume newVolume;
          std::tie(newVolume, std::ignore) =
              getVKLVolume(filename, vklVolumeType, requestedVarName);

          if (vklGetNumAttributes(newVolume) != 1) {
            throw std::runtime_error("requested grid " + varName +
                                     " is not a float volume");
          }

          VKLSampler newSampler = vklNewSampler(newVolume);
          vklCommit(newSampler);

          vklRelease(newVolume);

          return new OpenVKLVertexValueFloat(
              this, stats, newSampler, ValueMap());

        } catch (...) {
          RixContext *ctx = RixGetContext();
          RixMessages *m  = (RixMessages *)ctx->GetRixInterface(k_RixMessages);
          m->Error(
              "OpenVKLImplicitField: unable to bind vertex value for varName "
              "\"%s\"\n",
              requestedVarName.c_str());
        }
      }

    } else if (type == "color" || type == "vector" || type == "point" ||
               type == "normal") {
      try {
        VKLVolume newVolume;
        std::tie(newVolume, std::ignore) =
            getVKLVolume(filename, vklVolumeType, requestedVarName);

        if (vklGetNumAttributes(newVolume) != 3) {
          throw std::runtime_error("requested grid " + varName +
                                   " is not a vec3f volume");
        }

        VKLSampler newSampler = vklNewSampler(newVolume);
        vklCommit(newSampler);

        vklRelease(newVolume);

        return new OpenVKLVertexValueVec3f(this, stats, newSampler);

      } catch (...) {
        RixContext *ctx = RixGetContext();
        RixMessages *m  = (RixMessages *)ctx->GetRixInterface(k_RixMessages);
        m->Error(
            "OpenVKLImplicitField: unable to bind vertex value for varName "
            "\"%s\"\n",
            requestedVarName.c_str());
      }

    } else {
      RixContext *ctx = RixGetContext();
      RixMessages *m  = (RixMessages *)ctx->GetRixInterface(k_RixMessages);
      m->Error(
          "OpenVKLImplicitField: unable to bind vertex value for unsupported "
          "type \"%s\"\n",
          type.c_str());
    }

  } else {
    RixContext *ctx = RixGetContext();
    RixMessages *m  = (RixMessages *)ctx->GetRixInterface(k_RixMessages);
    m->Error(
        "OpenVKLImplicitField: unable to bind vertex value for variable "
        "\"%s\"\n",
        fullName.c_str());
  }

  return nullptr;
}

#ifdef OPENVKL_ENABLE_STATS
static void StatsReporter(void *ctx, class RixXmlFile *file)
{
  ((OpenVKLStats *)ctx)->ReportStats(file);
}
#endif

FIELDCREATE
{
  FIELDCREATE_ARGUSED;
  RixContext *ctx = RixGetContext();
  RixMessages *m  = (RixMessages *)ctx->GetRixInterface(k_RixMessages);

  if (nstring < 3) {
    m->Error(
        "Bad arguments to OpenVKLImplicitField plugin: expecting filename, "
        "requested VKL volume type, name of density field, [JSON dict]\n");
    return nullptr;
  }

  static bool initialized = false;

  if (!initialized) {
    vklLoadModule("cpu_device");

    m->Info("OpenVKLImplicitField: using Open VKL device: %s", OPENVKL_DEVICE);

    VKLDevice device = vklNewDevice(OPENVKL_DEVICE);

    vklCommitDevice(device);

    OpenVKLImplicitField::globalVKLDevice = device;

    initialized = true;
  }

  std::string filename(string[0]);
  std::string vklVolumeType(string[1]);
  std::string varName(string[2]);

  ValueMap densityMap;

  if (nstring >= 4 && string[3]) {
    float densityMult    = 1.f;
    float densityRolloff = 0.f;

    json j = json::parse(string[3]);

    if (j.contains("densityMult")) {
      densityMult = j["densityMult"];
    }

    if (j.contains("densityRolloff")) {
      densityRolloff = j["densityRolloff"];
    }

    densityMap = ValueMap(densityMult, densityRolloff);
  }

  varName = varName.substr(0, varName.find(":"));

  m->InfoAlways(
      "OpenVKLImplicitField: filename: %s, varName: %s, vklVolumeType: %s",
      filename.c_str(),
      varName.c_str(),
      vklVolumeType.c_str());

  // Register a singleton memory address with the renderer in which
  // we will accumulate global statistics
  OpenVKLStats *stats = nullptr;

#ifdef OPENVKL_ENABLE_STATS
  RixStorage *storageInterface =
      (RixStorage *)ctx->GetRixInterface(k_RixGlobalData);

  if (storageInterface) {
    storageInterface->Lock();
    const static RtUString vklStr("openvklStats");
    stats = (OpenVKLStats *)storageInterface->Get(vklStr);
    if (stats == nullptr) {
      stats = new OpenVKLStats();
      storageInterface->Set(vklStr, stats, nullptr);
      RixStats *statsInterface = (RixStats *)ctx->GetRixInterface(k_RixStats);
      statsInterface->AddReporterCtx(StatsReporter, stats);
    }
    storageInterface->Unlock();
  }
#endif

  try {
    OpenVKLImplicitField *implicitField = new OpenVKLImplicitField(
        stats, filename, vklVolumeType, varName, densityMap);
    return implicitField;
  } catch (const std::exception &e) {
    m->Error("OpenVKLImplicitField: caught exception: %s\n", e.what());
    return nullptr;
  }
}
