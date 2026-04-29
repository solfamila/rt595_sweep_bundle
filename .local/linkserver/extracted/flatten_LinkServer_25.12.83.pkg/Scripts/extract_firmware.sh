#!/bin/sh
###############################################################################
# Copyright 2024 NXP
#
# NXP Confidential and Proprietary. This software is owned or controlled by
# NXP and may only be used strictly in accordance with the applicable license
# terms.
#
# By expressly accepting such terms or by downloading, installing, activating
# and/or otherwise using the software, you are agreeing that you have read,
# and that you agree to comply with and are bound by, such license terms.
#
# If you do not agree to be bound by the applicable license terms, then you
# may not retain, install, activate or otherwise use the software.
#
###############################################################################
set -e

expanded_dir=$1
target_dir=$2

if [ -z $expanded_dir -o -z $target_dir ]; then
    exit 2
fi

pkgutil --expand-full $expanded_dir/flatten_LinkServer*.pkg/Payload/installers/MCU-LINK*.pkg $expanded_dir/tmp
mv $expanded_dir/tmp/flatten_MCU-LINK*.pkg/Payload/ $target_dir