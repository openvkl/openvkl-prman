// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <sstream>

#include <RixInterfaces.h>

#define STATS_NUM_BINS 10

class OpenVKLStats
{
 public:
  void accumulateEvals(int neval)
  {
    numEvals += neval;
    numEvalCallsBinned[getNumEvalsBin(neval)]++;
  }

  void ReportStats(RixXmlFile *file)
  {
    file->WriteText("\n");
    file->WriteXml(
        "<stats name=\"OpenVKL\" description=\"OpenVKL Implicit Plugin\" "
        "kind=\"section\">\n");

    file->WriteStat("numEvals", "Number of Evals", numEvals.load());

    for (int i = 0; i < numEvalCallsBinned.size(); i++) {
      std::stringstream ss1, ss2;

      ss1 << "numEvalCallsBinned" << i;

      int binMin = 1 << i;
      int binMax = 1 << (i + 1);

      if (i == numEvalCallsBinned.size() - 1) {
        ss2 << "Number of calls for [" << binMin << ", inf) nevals";
      } else {
        ss2 << "Number of calls for [" << binMin << ", " << binMax
            << ") nevals";
      }

      file->WriteStat(
          ss1.str().c_str(), ss2.str().c_str(), numEvalCallsBinned[i]);
    }

    file->WriteXml("</stats>\n");
  }

 private:
  int getNumEvalsBin(int neval)
  {
    assert(neval > 0);
    return std::min(int(std::log2(neval)), STATS_NUM_BINS - 1);
  }

  std::atomic<uint64_t> numEvals;
  std::array<std::atomic<uint64_t>, STATS_NUM_BINS> numEvalCallsBinned;
};
