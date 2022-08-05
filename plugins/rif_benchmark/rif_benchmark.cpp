// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <regex>
#include <string>

#include "RifPlugin.h"
#include "ri.h"

/*
 * RIB filter plugin that replaces volume types used in our scene files
 * for benchmark / comparison purposes. This plugin is able to take volumes
 * specified with the `impl_openvdb` parameterization, and convert them to use
 * the Open VKL plugin instead.
 *
 * See README.md for documentation and examples.
 */
class BenchmarkFilter : public RifPlugin
{
 public:
  BenchmarkFilter(const std::string &plugin,
                  const std::string &type,
                  bool filterDisplay,
                  bool interactive);
  RifFilter &GetFilter() override;

 private:
  static RtVoid filterDisplayV(const char *name,
                               RtToken type,
                               RtToken mode,
                               int numArgs,
                               RtToken *argToken,
                               RtPointer *argValue);

  static RtVoid convertVolumeToVdb(RtToken type,
                                   RtBound bound,
                                   int *dimensions,
                                   int numArgs,
                                   RtToken *argToken,
                                   RtPointer *argValue);

  static RtVoid convertVolumeToVkl(RtToken type,
                                   RtBound bound,
                                   int *dimensions,
                                   int numArgs,
                                   RtToken *argToken,
                                   RtPointer *argValue);

 private:
  RifFilter rifFilter;
  std::string prefix;
  std::string plugin;
  std::string type;
  bool filterDisplay;
  bool interactive;
};

BenchmarkFilter::BenchmarkFilter(const std::string &plugin,
                                 const std::string &type,
                                 bool filterDisplay,
                                 bool interactive)
    : plugin(plugin),
      type(type),
      filterDisplay(filterDisplay),
      interactive(interactive)
{
  if (filterDisplay) {
    rifFilter.DisplayV = filterDisplayV;
  }

  if (std::regex_search(plugin, std::regex("openvdb"))) {
    rifFilter.VolumeV = convertVolumeToVdb;
    prefix            = "openvdb";
  } else if (std::regex_search(plugin, std::regex("openvkl"))) {
    rifFilter.VolumeV = convertVolumeToVkl;
    prefix            = "openvkl_" + type;
  }

  // Continue by default.
  rifFilter.Filtering = RifFilter::k_Continue;
}

RifFilter &BenchmarkFilter::GetFilter()
{
  return rifFilter;
}

RtVoid BenchmarkFilter::filterDisplayV(const char *name,
                                       RtToken type,
                                       RtToken mode,
                                       int numArgs,
                                       RtToken *argToken,
                                       RtPointer *argValue)
{
  BenchmarkFilter *filter =
      static_cast<BenchmarkFilter *>(RifGetCurrentPlugin());

  const std::string fileName = filter->prefix + ".tif";
  RiDisplayV(fileName.c_str(), "tiff", "rgba", numArgs, argToken, argValue);
  if (filter->interactive)
    RiDisplayV(
        filter->prefix.c_str(), "it", "rgba", numArgs, argToken, argValue);
}

RtVoid BenchmarkFilter::convertVolumeToVdb(RtToken type,
                                           RtBound bound,
                                           int *dimensions,
                                           int numArgs,
                                           RtToken *argToken,
                                           RtPointer *argValue)
{
  BenchmarkFilter *filter =
      static_cast<BenchmarkFilter *>(RifGetCurrentPlugin());

  const std::string typeStr = "blobbydso:" + filter->plugin;

  std::vector<const char *> stringargs;
  std::vector<RtToken> newArgToken(numArgs, nullptr);
  std::vector<RtPointer> newArgValue(numArgs, nullptr);

  std::regex pattern("string\\[(\\d)\\] blobbydso:stringargs");

  for (int i = 0; i < numArgs; ++i) {
    const std::string s(argToken[i]);
    std::smatch match;

    if (std::regex_search(s, match, pattern)) {
      const int numArgValues = std::stoi(match[1]);

      // args provided: file, density, [velocity], [JSON args]
      // args output: <same>
      newArgToken[i] = "constant string[4] blobbydso:stringargs";

      for (int j = 0; j < numArgValues; ++j) {
        stringargs.push_back(static_cast<RtString *>(argValue[i])[j]);
      }

      for (int j = numArgValues; j < 4; ++j) {
        stringargs.push_back(nullptr);
      }

      newArgValue[i] = stringargs.data();
    } else {
      newArgToken[i] = argToken[i];
      newArgValue[i] = argValue[i];
    }
  }

  RiVolumeV(typeStr.c_str(),
            bound,
            dimensions,
            numArgs,
            newArgToken.data(),
            newArgValue.data());
}

RtVoid BenchmarkFilter::convertVolumeToVkl(RtToken type,
                                           RtBound bound,
                                           int *dimensions,
                                           int numArgs,
                                           RtToken *argToken,
                                           RtPointer *argValue)
{
  BenchmarkFilter *filter =
      static_cast<BenchmarkFilter *>(RifGetCurrentPlugin());

  const std::string typeStr = "blobbydso:" + filter->plugin;

  std::vector<const char *> stringargs;
  std::vector<RtToken> newArgToken(numArgs, nullptr);
  std::vector<RtPointer> newArgValue(numArgs, nullptr);

  std::regex pattern("string\\[(\\d)\\] blobbydso:stringargs");

  for (int i = 0; i < numArgs; ++i) {
    const std::string s(argToken[i]);
    std::smatch match;

    if (std::regex_search(s, match, pattern)) {
      const int numArgValues = std::stoi(match[1]);

      // args provided: file, density, [velocity], [JSON args]
      // args output: file, type, density, [JSON args]; velocity not used!
      newArgToken[i] = "constant string[4] blobbydso:stringargs";

      if (numArgValues < 2) {
        throw std::runtime_error(
            "require at least 2 input blobbydso:stringargs");
      }

      stringargs.push_back(static_cast<RtString *>(argValue[i])[0]);  // file
      stringargs.push_back(filter->type.c_str());                     // type
      stringargs.push_back(static_cast<RtString *>(argValue[i])[1]);  // density

      if (numArgValues >= 3 &&
          !std::string(static_cast<RtString *>(argValue[i])[2]).empty()) {
        throw std::runtime_error(
            "velocity argument not supported for Open VKL plugin");
      }

      stringargs.push_back(
          numArgValues < 4
              ? nullptr
              : static_cast<RtString *>(argValue[i])[3]);  // [JSON args]

      newArgValue[i] = stringargs.data();
    } else {
      newArgToken[i] = argToken[i];
      newArgValue[i] = argValue[i];
    }
  }

  RiVolumeV(typeStr.c_str(),
            bound,
            dimensions,
            numArgs,
            newArgToken.data(),
            newArgValue.data());
}

// -----------------------------------------------------------------------------
// PRman API.
// -----------------------------------------------------------------------------
extern "C" RifPlugin *RifPluginManufacture(int argc, char **argv)
{
  std::string plugin;
  std::string type;
  bool filterDisplay{false};
  bool interactive{false};

  for (; argc > 0 && argv[0][0] == '-'; argc--, argv++) {
    if (!strncmp("-plugin", argv[0], 7) && argc > 1) {
      argc--;
      argv++;
      plugin = std::string(argv[0]);
    }

    else if (!strncmp("-type", argv[0], 7) && argc > 1) {
      argc--;
      argv++;
      type = std::string(argv[0]);
    }

    else if (!strcmp("-filterDisplay", argv[0]))
      filterDisplay = true;

    else if (!strcmp("-it", argv[0])) {
      filterDisplay = true;
      interactive   = true;
    }
  }

  return new BenchmarkFilter(plugin, type, filterDisplay, interactive);
}
