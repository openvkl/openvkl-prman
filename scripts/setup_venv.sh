#!/bin/bash
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

SCRIPT_DIRECTORY="$(dirname '$0')"

VENV_DIRECTORY="${SCRIPT_DIRECTORY}/venv"

pushd ${SCRIPT_DIRECTORY}

python3 -m venv venv
. venv/bin/activate
pip install tabulate

popd

echo

if [ ! -d "${VENV_DIRECTORY}" ]; then
  echo "failed to setup venv"
  return 1
fi

echo "successfully setup venv at ${VENV_DIRECTORY}; run 'source venv.sh' to activate it"
