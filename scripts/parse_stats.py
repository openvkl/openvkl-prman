#!/usr/bin/env python3
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

import argparse
import xml.etree.ElementTree as ET
import tabulate

def parse_stats_xml(filename):
    d = {'filename': filename}

    root = ET.parse(filename).getroot()
    d['implicitPluginTimers'] = root.find('.//timer[@name="implicitPluginTimers"]/elapsed').text
    d['totaltime'] = root.find('.//timer[@name="totaltime"]/elapsed').text

    return d

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument("stats_xml_filename", nargs='+')
    args = parser.parse_args()

    stats_all = [parse_stats_xml(f) for f in args.stats_xml_filename]

    headers = ['filename', 'implicitPluginTimers', 'totaltime']
    content = []

    for stats in stats_all:
      content.append([stats[h] for h in headers])

    print(tabulate.tabulate(content, headers=headers))
