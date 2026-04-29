#!/usr/bin/env rltool 

# Copyright 2020 NXP

# Utility to print the content of a trace and analog format file as generated
# by MCU-Link

# syntax: rltool rdtadp.rl <filename>

# Why is this application structured in this slightly strange way?
#
# rltool generates a lot of garbage as it executes functions and this
# garbage is collected (returned to the system) only when the next command
# line is requested.
# This means that functions that execute forever ... or for a very long time
# can generate large amounts of un-collected garbage which can lead to system
# memory running low.
# The 'every' command is provided to help - it runs a command line over and
# over until the value it generates is FALSE.  Normally it sleeps for a number
# of milliseconds between every run, but if the number of milliseconds is set
# to -1 the command line is simply executed repeatedly.
# This will work only when the command is called as a command (not from an
# rltool function) so we have to do the main argument parsing and setup on
# the command line.
#
# So this application
#    - parses command line arguments
#    - defines a function to process each "block" of input
#    - calls the function using 'every -1'

set id "rdtadp"

if parse.argv.0 == "rltool" {
    printf "$id: please run script without entering rltool command line mode\n" <>!;
    exit!;
}{}

# print parse.argv

set myhelp[]:{
    printf "syntax: <trace_analog_file>\n"<>!;
}

set getargs[arg,argv]:{
    .n = 1;
    (len argv!) _gt_ n {
        arg.file = argv.(n);
        n = n+1;
    }!;
    inenv arg "file"!
}

set arg []
if (not (getargs arg parse.argv!)) {
    myhelp!;
    exit!
}{}

# print arg

set prot io.binfile arg.file "r"!

if prot == NULL {
    printf "$id: can't open %v to read\n" <arg.file>!;
    exit!
}{}

set BINPROT_PKTID [
    SYNC      = 0x00010001,
    SWO       = 0x00020001,
    SWOTIME   = 0x00020002,
    ALOGVAL   = 0x00030001,
    ALOGAVG   = 0x00030002,
]

set bytepos 0

set getblock[inf]:{
    # echo "rd 8"!;
    .hdr = io.read inf 8!;
    if (hdr != NULL {(len hdr!) == 8}!) {
        .hdrint = binsplit TRUE FALSE 4 hdr!;
        .size = hdrint.0;
        .blkid = hdrint.1;
        .pktmatch =
            select [val]:{val == blkid} BINPROT_PKTID!;
        #print pktmatch!;
        .pkttype = if (pktmatch != NULL {(len pktmatch!) == 1}!) {
                       (domain pktmatch!).0} {"UNKNOWN"}!;
        #print pkttype!;
        printf "%07d: Block %08X (%s) [%d]\n"<bytepos, blkid, pkttype, size>!;
        bytepos = bytepos+8;
        # echo "rd blk"!;
        .data = io.read inf (size-8)!;
        .datalen = len data!;
        bytepos = bytepos+datalen;
    }{
        printf "%07d: EOF\n"<bytepos>!;
        FALSE
    }!  
}


#print prot

every -1 getblock prot

io close prot

