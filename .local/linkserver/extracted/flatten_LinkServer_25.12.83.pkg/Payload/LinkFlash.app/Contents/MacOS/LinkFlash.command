#!/bin/sh

# Copyright 2024 NXP
# SPDX-License-Identifier: BSD-3-Clause

FILEPATH=`dirname $0`
BASEPATH=${FILEPATH%/*/*/*}
LINKSERVER=$BASEPATH/LinkServer

$LINKSERVER gui flash