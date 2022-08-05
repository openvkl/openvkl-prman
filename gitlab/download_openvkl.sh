#!/bin/bash -xe
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if [ "$#" -ne 2 ]; then
  echo "usage: $0 <branch> <job name>"
  exit 1
fi

OPENVKL_PROJECT_ID="20"
BRANCH=$1
JOB_NAME=$2

URL="http://vis-gitlab.an.intel.com/api/v4/projects/${OPENVKL_PROJECT_ID}/jobs/artifacts/${BRANCH}/download?job=${JOB_NAME}"

mkdir openvkl
cd openvkl

# note, CI_JOB_TOKEN is not functional for the API, so we must use a private token
curl --location --header "PRIVATE-TOKEN: ${OPENVKL_CI_TOKEN}" --output artifacts.zip "${URL}"

unzip artifacts.zip
