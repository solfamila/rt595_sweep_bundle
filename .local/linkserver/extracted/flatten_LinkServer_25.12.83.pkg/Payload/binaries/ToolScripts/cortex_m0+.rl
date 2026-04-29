# Copyright 2020 NXP

# Built-in register support for a Cortex M0+


# To keep name spaces clean use this file as follows:
# set cm parse.rdmod "thisfilename"!
# Then access its components as
#     cm.dev.<d>.<f>  - e.g. cm.dev.TPIU.TPIU_FFCR, for register definitions
#     cm.fld.<d>.<f>  - e.g. cm.fld.TPIU.TPIU_FFCR, for field definitions
#     cm.rom.<d>      - e.g. cm.rom.TPIU, for potential ROM addresses

set dev []   # device as set of registers definition repository
set bse []   # (guesses) for base addresses for devices listed in ROM
set fld []   # field definition repository


# No ITM, ETM or TPIU support in ARMv6



# System control space registers - Core debug control regisers
# normally present with breakpoint control registers and
# data watchpoint trace (DWT) peripherals
set dev.SCS[rd,wr]:{[
    ACTLR    = <0x008,rd,rd,wr>,

    STCSR    = <0x010,rd,rd,wr>,
    STRVR    = <0x014,rd,rd,wr>,
    STCVR    = <0x018,rd,rd,wr>,
    STCR     = <0x01C,rd,rd,NULL>, 

    CPUID    = <0xD00,rd,rd,NULL>,
    ICSR     = <0xD04,rd,rd,wr>,
    VTOR     = <0xD08,rd,rd,wr>,
    AIRCR    = <0xD0C,rd,rd,wr>,
    SCR      = <0xD10,rd,rd,wr>,
    CCR      = <0xD14,rd,rd,wr>,
#   SHPR1    = <0xD18,rd,rd,wr>,
    SHPR2    = <0xD1C,rd,rd,wr>,
    SHPR3    = <0xD20,rd,rd,wr>,
    SHCSR    = <0xD24,rd,rd,wr>,
#   CFSR     = <0xD28,rd,rd,wr>,
#   HFSR     = <0xD2C,rd,rd,wr>,
    DFSR     = <0xD30,rd,rd,wr>,
#   MMFAR    = <0xD34,rd,rd,wr>,
#   BFAR     = <0xD38,rd,rd,wr>,
#   AFSR     = <0xD3C,rd,rd,wr>, 

    ID_PFR0  = <0xD40,rd,rd,NULL>,
    ID_PFR1  = <0xD44,rd,rd,NULL>,
    ID_DFR0  = <0xD48,rd,rd,NULL>,
    ID_AFR0  = <0xD4C,rd,rd,NULL>,
    ID_MMFR0 = <0xD50,rd,rd,NULL>,
    ID_MMFR1 = <0xD54,rd,rd,NULL>,
    ID_MMFR2 = <0xD58,rd,rd,NULL>,
    ID_MMFR3 = <0xD5C,rd,rd,NULL>,
    ID_ISAR0 = <0xD60,rd,rd,NULL>,
    ID_ISAR1 = <0xD64,rd,rd,NULL>,
    ID_ISAR2 = <0xD68,rd,rd,NULL>,
    ID_ISAR3 = <0xD6C,rd,rd,NULL>,
    ID_ISAR4 = <0xD70,rd,rd,NULL>,

    CPACR    = <0xD88,rd,rd,wr>,

    MPU_TYPE = <0xD90,rd,rd,NULL>,
    MPU_CTRL = <0xD94,rd,rd,wr>,
    MPU_RNR  = <0xD98,rd,rd,wr>,
    MPU_RBAR = <0xD9C,rd,rd,wr>,
    MPU_RASR = <0xDA0,rd,rd,wr>,

    STIR     = <0xF00,NULL,NULL,wr>,

    DHCSR    = <0xDF0,rd,rd,wr>,
    DCRSR    = <0xDF4,NULL,NULL,wr>,
    DCRDR    = <0xDF8,rd,rd,wr>,
    DEMCR    = <0xDFC,rd,rd,wr>,
    ]
}


set fld.SCS [
    DHCSR      = [
        C_DEBUGEN    = <0,1,bitmsg>,
        C_HALT       = <1,1,bitmsg>,
        C_STEP       = <2,1,bitmsg>,
        C_MASKINTS   = <3,1,bitmsg>,
        S_REGRDY     = <16,1,bitmsg>,
        S_HALT       = <17,1,bitmsg>,
        S_SLEEP      = <18,1,bitmsg>,
        S_LOCKUP     = <19,1,bitmsg>,
        S_RETIRE_ST  = <24,1,bitmsg>,
        S_RESET_ST   = <25,1,bitmsg>,
    ],
    DEMCR      = [
        VC_CORERESET = <0,1,bitmsg>,
        VC_HARDERR   = <10,1,bitmsg>,
        TRCENA       = <24,1,bitmsg>, # DWTENA
    ],
]


# Data Watchpoint Trace registers
set dev.DWT[rd,wr]:{[
    DWT_CTRL      = <0x000,rd,rd,NULL>,
#   DWT_CYCCNT    = <0x004,rd,rd,wr>,
#   DWT_CPICNT    = <0x008,rd,rd,wr>,
#   DWT_EXCCNT    = <0x00C,rd,rd,wr>,
#   DWT_SLEEPCNT  = <0x010,rd,rd,wr>,
#   DWT_LSUCNT    = <0x014,rd,rd,wr>,
#   DWT_FOLDCNT   = <0x018,rd,rd,wr>,
    DWT_PCSR      = <0x01C,rd,rd,NULL>,
    DWT_COMP0     = <0x020,rd,rd,wr>,
    DWT_MASK0     = <0x024,rd,rd,wr>,
    DWT_FUNCTION0 = <0x028,rd,rd,wr>,
    DWT_COMP1     = <0x030,rd,rd,wr>,
    DWT_MASK1     = <0x034,rd,rd,wr>,
    DWT_FUNCTION1 = <0x038,rd,rd,wr>,
    DWT_COMP2     = <0x040,rd,rd,wr>,
    DWT_MASK2     = <0x044,rd,rd,wr>,
    DWT_FUNCTION2 = <0x048,rd,rd,wr>,
    DWT_COMP3     = <0x050,rd,rd,wr>,
    DWT_MASK3     = <0x054,rd,rd,wr>,
    DWT_FUNCTION3 = <0x058,rd,rd,wr>,
    ]
}


set local.fld_DWT_FUNCTION [
        FUNCTION      = <0,4,[n]:{
                            if n == 0 {"disabled"}{
                                strf "if DATAVMATCH %s, else if CYCMATCH %s, else %s"
                                <
                                   { .msg = "<unpredictable>";
                                     n==5 {msg = "watchpoint on read"}!;
                                     n==6 {msg = "watchpoint on write"}!;
                                     n==7 {msg = "watchpoint on read/write"}!;
                                     n==9 {msg = "CMPMATCHIN on read"}!;
                                     n==10 {msg = "CMPMATCHIN on write"}!;
                                     n==11 {msg = "CMPMATCHIN on read/write"}!;
                                     msg
                                   }!,
                                   { .msg = "<unpredictable>";
                                     n==0 {msg = "comparitor disabled"}!;
                                     n==1 {msg = "PC value trace"}!;
                                     n==4 {msg = "watchpoint"}!;
                                     n==8 {msg = "CMPMATCHIN"}!;
                                     msg
                                   }!,
                                   choicemsg <
                                       "comparitor disabled", # 0
                                       "PC value/address trace on read/write",
                                       "data(+address) trace on read/write",
                                       "PC value(address) and data trace on read/write",
                                       "PC watchpoint on instr", # 4
                                       "watchpoint on read",
                                       "watchpoint on write", 
                                       "watchpoint on read/write",
                                       "CMPMATCHIN on instr", # 8
                                       "CMPMATCHIN on read",
                                       "CMPMATCHIN on write",
                                       "CMPMATCHIN on read/write",
                                       "data(address) trace on read", #12
                                       "data(address) trace on write",
                                       "PC value(address) trace on read",
                                       "PC value(address) trace on write"
                                   > n!
                                >!
                            }!
                        }>,
        MATCHED       = <24,1,bitmsg>,
]

set fld.DWT [
    # Arm v7:
    DWT_CTRL      = [
        NUMCOMP       = <28,4,intmsg "%d comparators">,
    ],
    DWT_FUNCTION0 = local.fld_DWT_FUNCTION,
    DWT_FUNCTION1 = local.fld_DWT_FUNCTION,
    DWT_FUNCTION2 = local.fld_DWT_FUNCTION,
    DWT_FUNCTION3 = local.fld_DWT_FUNCTION,
]



# Flash Breakpoint regisers
set dev.FBP[rd,wr]:{[
    FB_CTRL      = <0x000,rd,rd,wr>,
    FB_REMAP     = <0x004,rd,rd,wr>,
    FB_COMP0     = <0x008,rd,rd,wr>,
    FB_COMP1     = <0x00C,rd,rd,wr>,
    FB_COMP2     = <0x010,rd,rd,wr>,
    FB_COMP3     = <0x014,rd,rd,wr>,
    FB_COMP4     = <0x018,rd,rd,wr>,
    FB_COMP5     = <0x01C,rd,rd,wr>,
    FB_COMP6     = <0x020,rd,rd,wr>,
    FB_COMP7     = <0x024,rd,rd,wr>,
    ]
}



set bse [
#   ITM = 0xE0000000,
    SCS = 0xE000E000,
    DWT = 0xE0001000,
    FBP = 0xE0002000,
#   TPIU = 0xE0040000,
#   ETM = 0xE0041000
]
