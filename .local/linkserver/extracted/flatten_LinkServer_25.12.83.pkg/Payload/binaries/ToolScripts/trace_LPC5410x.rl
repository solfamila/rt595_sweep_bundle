# Copyright 2020 NXP

# rltool - SWO trace settings file for LPC5410x

set LPC5410x_TRACECLKDIV 0x400000E4

# LPC5410x only - set TRACECLKDIV to 1
set LPC5410x_on_trace_start[apbus]:{
    # echo "LPC5410x trace enabled"!;
    apbus.wr32 LPC5410x_TRACECLKDIV 1!;
}


# LPC5410x only - set TRACECLKDIV to 0
set LPC5410x_on_trace_stop[apbus]:{
    apbus.wr32 LPC5410x_TRACECLKDIV 0!;
}

set opt.on_trace_start LPC5410x_on_trace_start
set opt.on_trace_stop  LPC5410x_on_trace_stop


# Set the typical CPU frequency of LPC5410x if unset

if opt.cpufreq == NULL {
    echo "CPU frequency set!"!;
    opt.cpufreq = 96000000;
}{}
