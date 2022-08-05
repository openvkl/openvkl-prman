#!/bin/bash
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

SCRIPT_DIRECTORY="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

VENV_DIRECTORY="${SCRIPT_DIRECTORY}/venv"

if [ ! -d "${VENV_DIRECTORY}" ]; then
  echo "venv has not been setup; run the 'setup_venv.sh' script to create it."
  return 1
fi

. ${VENV_DIRECTORY}/bin/activate

export PATH=${SCRIPT_DIRECTORY}:${PATH}
