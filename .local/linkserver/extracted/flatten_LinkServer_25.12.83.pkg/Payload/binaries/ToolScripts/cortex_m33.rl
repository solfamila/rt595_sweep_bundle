# Copyright 2020 NXP

# Built-in register support for a Cortex M33

# To keep name spaces clean use this file as follows:
# set cm parse.rdmod "thisfilename"!
# Then access its components as
#     cm.dev.<d>.<f>  - e.g. cm.dev.TPIU.TPIU_FFCR, for register definitions
#     cm.fld.<d>.<f>  - e.g. cm.fld.TPIU.TPIU_FFCR, for field definitions
#     cm.rom.<d>      - e.g. cm.rom.TPIU, for potential ROM addresses

set dev []   # device as set of registers definition repository
set bse []   # (guesses) for base addresses for devices listed in ROM
set fld []   # field definition repository


# Definition of registers in a TPIU peripheral on a Cortex M7
# Must be given functions to read from an arbitrary address and to write to one
# Each entry has
#   (0) - offset from the peripheral base
#   (1) - function to read from register
#   (2) - function to read from register only used for enumeration
#   (3) - function to write to register
# If a register has side-effects when read provide NULL for (2)
# If a register is read-only provide NULL for (3)
# If a register is write-only provide NULL for (1) and (2)
set dev.TPIU[rd,wr]:{[
    # Cortex M33:
    TPIU_SSPSR = <0x000,rd,rd,NULL>,
    TPIU_CSPSR = <0x004,rd,rd,wr>,
    TPIU_ACPR  = <0x010,rd,rd,wr>,
    TPIU_SPPR  = <0x0F0,rd,rd,wr>,
    TPIU_FFSR  = <0x300,rd,rd,NULL>,
    TPIU_FFCR  = <0x304,rd,rd,wr>,
    TPIU_FSCR  = <0x308,rd,rd,wr>,
    TRIGGER    = <0xEE8,rd,rd,NULL>,
    FIFO0      = <0xEEC,rd,rd,NULL>,
    ITATBCTR2  = <0xEF0,rd,rd,NULL>,
    ITATBCTR0  = <0xEF8,rd,rd,NULL>,
    FIFO1      = <0xEFC,rd,rd,NULL>, # renamed to ITFTTD1 in M33 spec
    ITCTRL     = <0xF00,rd,rd,wr>,
    CLAIMSET   = <0xFA0,rd,rd,wr>,
    CLAIMCLR   = <0xFA4,rd,rd,wr>,
    DEVID      = <0xFC8,rd,rd,NULL>,
    ]
}

set fld.TPIU [
    TPIU_SPPR = [
        TXMODE    = <0,2,choicemsg <"parallel trace port mode",
                                    "asynchronous SWO using Manchester encoding",
                                    "asynchronous SWO, using NRZ encoding",
                                    "<reserved>">>,
    ],
    DEVID = [
        TRINCNT   = <0,6,[n]:{strf "%d trace inputs"<1_shl_n>!}>,
        FIFOSZ    = <6,3,[n]:{strf "min buf %d bytes"<1_shl_n>!}>,
        PTINVALID = <9,1,choicemsg <"parallel tracce supported",
                                    "parallel trace not supported">>,
        MANCVALID = <10,1,choicemsg <"SWO (Manchester) not supported",
                                    "SWO (Manchester) supported">>,
        NRZVALID  = <11,1,choicemsg <"SWO (NRZ) not supported",
                                     "SWO (NRZ) supported">>,
    ],
]


# Definition of registers in an ETM peripheral on a Coretex M7
# Embedded Trace Macrocell
set dev.ETM[rd,wr]:{[
    # Cortex M3:
    ETMCR         = <0x000,rd,rd,wr>,
    ETMCCR        = <0x004,rd,rd,NULL>,
    ETMTRIGGER    = <0x008,rd,rd,wr>,
    ETMSR         = <0x010,rd,rd,wr>,
    ETMSCR        = <0x014,rd,rd,NULL>,
    ETMTEEVR      = <0x020,rd,rd,wr>,
    ETMTECR1      = <0x024,rd,rd,wr>,
    ETMFFLR       = <0x028,rd,rd,wr>,
    ETMSYNCFR     = <0x1E0,rd,rd,NULL>,
    ETMIDR        = <0x1E4,rd,rd,NULL>,
    ETMCCER       = <0x1E8,rd,rd,NULL>,
    ETMTESSEICR   = <0x1F0,rd,rd,wr>,
    ETMTRACEIDR   = <0x200,rd,rd,wr>,
    ETMIDR2       = <0x208,rd,rd,NULL>,
    ETMOSLAR      = <0x300,rd,rd,wr>,
    ETMOSLSR      = <0x304,rd,rd,NULL>,
    ETMOSSRR      = <0x308,rd,rd,wr>,
    ETMPDCR       = <0x310,rd,rd,wr>,
    ETMPDSR       = <0x314,rd,rd,NULL>,
    ITMISCIN      = <0xEE0,rd,rd,NULL>,
    ITTRIGOUT     = <0xEE4,NULL,NULL,wr>,
    ETM_ITATBCTR2 = <0xEE8,rd,rd,NULL>,
    ETM_ITATBCTR0 = <0xEE8,NULL,NULL,wr>,
    ETMITCTRL     = <0xF00,rd,rd,wr>,
    ETMCLAIMSET   = <0xFA0,rd,rd,wr>,
    ETMCLAIMCLR   = <0xFA4,rd,rd,wr>,
    ETMCLAR       = <0xFB0,rd,rd,wr>,
    ETMCLSR       = <0xFB0,rd,rd,NULL>,
    ETMAUTHSTATUS = <0xFB8,rd,rd,NULL>,
    ETMDEVTYPE    = <0xFCC,rd,rd,NULL>,
    ]
}


# Definition of registers in an ITM peripheral on a Coretex M7
set dev.ITM[rd,wr]:{[
    # Cortex M3:
    ITM_STIM0  = <0x000,rd,rd,wr>,
    ITM_STIM1  = <0x004,rd,rd,wr>,
    ITM_STIM2  = <0x008,rd,rd,wr>,
    ITM_STIM3  = <0x00C,rd,rd,wr>,
    ITM_STIM4  = <0x010,rd,rd,wr>,
    ITM_STIM5  = <0x014,rd,rd,wr>,
    ITM_STIM6  = <0x018,rd,rd,wr>,
    ITM_STIM7  = <0x01C,rd,rd,wr>,
    ITM_STIM8  = <0x020,rd,rd,wr>,
    ITM_STIM9  = <0x024,rd,rd,wr>,
    ITM_STIM10 = <0x028,rd,rd,wr>,
    ITM_STIM11 = <0x02C,rd,rd,wr>,
    ITM_STIM12 = <0x030,rd,rd,wr>,
    ITM_STIM13 = <0x034,rd,rd,wr>,
    ITM_STIM14 = <0x038,rd,rd,wr>,
    ITM_STIM14 = <0x03C,rd,rd,wr>,
    ITM_STIM16 = <0x040,rd,rd,wr>,
    ITM_STIM17 = <0x044,rd,rd,wr>,
    ITM_STIM18 = <0x048,rd,rd,wr>,
    ITM_STIM19 = <0x04C,rd,rd,wr>,
    ITM_STIM20 = <0x050,rd,rd,wr>,
    ITM_STIM21 = <0x054,rd,rd,wr>,
    ITM_STIM22 = <0x058,rd,rd,wr>,
    ITM_STIM23 = <0x05C,rd,rd,wr>,
    ITM_STIM24 = <0x060,rd,rd,wr>,
    ITM_STIM25 = <0x064,rd,rd,wr>,
    ITM_STIM26 = <0x068,rd,rd,wr>,
    ITM_STIM27 = <0x06C,rd,rd,wr>,
    ITM_STIM28 = <0x070,rd,rd,wr>,
    ITM_STIM29 = <0x074,rd,rd,wr>,
    ITM_STIM30 = <0x078,rd,rd,wr>,
    ITM_STIM31 = <0x07C,rd,rd,wr>,

    ITM_TER0   = <0xE00,rd,rd,wr>,
    # Arm v7:
    ITM_TER1   = <0xE04,rd,rd,wr>,
    ITM_TER2   = <0xE08,rd,rd,wr>,
    ITM_TER3   = <0xE0C,rd,rd,wr>,
    ITM_TER4   = <0xE10,rd,rd,wr>,
    ITM_TER5   = <0xE14,rd,rd,wr>,
    ITM_TER6   = <0xE18,rd,rd,wr>,
    ITM_TER7   = <0xE20,rd,rd,wr>,
    ITM_TPR    = <0xE40,rd,rd,wr>,
    ITM_TCR    = <0xE80,rd,rd,wr>,

    # Cortex M7:
    ITM_ITATRDY = <0xEF0,rd,rd,NULL>,
    ITM_ITAVAL  = <0xEF8,NULL,NULL,wr>,
    ITM_TCTRL   = <0xF00,NULL,NULL,wr>,
    ITM_LAR     = <0xFB0,NULL,NULL,wr>,
    ITM_LSR     = <0xFB4,rd,rd,NULL>,
    ]
}


set fld.ITM [
    # Arm v7
    ITM_TCR = [
        ITMENA      = <0,1,endismsg>,
        TSENA       = <1,1,endismsg>,
        SYNCENA     = <2,1,endismsg>,
        TXENA       = <3,1,endismsg>,
        SWOENA      = <4,1,boolmsg "processor clock" "TPIU clock">,
        TSPrescale  = <8,2,choicemsg <"no prescale","div by 4","div by 16","div by 64">>,
        GTSFREQ     = <10,2,choicemsg <"never","every 128 cycles","every 8192 cycles","every pkt">>,
        TraceBusID  = <16,7,bitmsg>,
        BUSY        = <23,1,bitmsg>,
    ],
    # cortext M7
    ITM_TPR = [
        PRIVMASK    = <0,4,[n]:{
                          strf "ports [7:0] %sabled, [15:8] %sabled, [23:16] %sabled, [31:24] %sabled"<
                             if (n _bitand_ 0x1) == 0 {"dis"}{"en"}!,
                             if (n _bitand_ 0x2) == 0 {"dis"}{"en"}!,
                             if (n _bitand_ 0x4) == 0 {"dis"}{"en"}!,
                             if (n _bitand_ 0x8) == 0 {"dis"}{"en"}!,
                             >!}>,
    ],
]

# System control space registers for a Cortex M7 - Core debug control regisers
# normally present with breakpoint control registers and
# data watchpoint trace (DWT) peripherals
set dev.SCS[rd,wr]:{[
    # Cortex M33 - all registers have privileged access:
    ACTLR    = <0x008,rd,rd,wr>,

    SYST_CSR   = <0x010,rd,rd,wr>,    # alias of M3 STCSR
    SYST_RVR   = <0x014,rd,rd,wr>,    # alias of M3 STRVR
    SYST_CVR   = <0x018,rd,rd,wr>,    # alias of M3 STCVR
    SYST_CALIB = <0x01C,rd,rd,NULL>,  # alias of M3 STCR

    CPUID    = <0xD00,rd,rd,NULL>,
    ICSR     = <0xD04,rd,rd,wr>,
    VTOR     = <0xD08,rd,rd,wr>,
    AIRCR    = <0xD0C,rd,rd,wr>,
    SCR      = <0xD10,rd,rd,wr>,
    CCR      = <0xD14,rd,rd,wr>,
    SHPR1    = <0xD18,rd,rd,wr>,
    SHPR2    = <0xD1C,rd,rd,wr>,
    SHPR3    = <0xD20,rd,rd,wr>,
    SHCSR    = <0xD24,rd,rd,wr>,
    CFSR     = <0xD28,rd,rd,wr>, # MMFSR, BFSR, UFSR are subsets
    HFSR     = <0xD2C,rd,rd,wr>,
    DFSR     = <0xD30,rd,rd,wr>,
    MMFAR    = <0xD34,rd,rd,wr>,
    BFAR     = <0xD38,rd,rd,wr>,
    AFSR     = <0xD3C,rd,rd,wr>, 

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
    NSACR    = <0xD8C,rd,rd,wr>,

    # SAU - Security Attribution Unit
    SAU_CTRL = <0xDD0,rd,rd,wr>,
    SAU_TYPE = <0xDD4,rd,rd,NULL>,
    SAU_RNR  = <0xDD8,rd,rd,wr>,
    SAU_RBAR = <0xDDC,rd,rd,wr>,
    SAU_RLAR = <0xDE0,rd,rd,wr>,
    SFSR     = <0xDE4,rd,rd,wr>,
    SFAR     = <0xDE8,rd,rd,wr>,

    # following don't appear in the on-line M33 manual
    STIR     = <0xF00,NULL,NULL,wr>,

    DHCSR    = <0xDF0,rd,rd,wr>,
    DCRSR    = <0xDF4,NULL,NULL,wr>,
    DCRDR    = <0xDF8,rd,rd,wr>,
    DEMCR    = <0xDFC,rd,rd,wr>,

    # Cortex M7:
    CLIDR      = <0xD78,rd,rd,NULL>,
    CTR        = <0xD7C,rd,rd,NULL>,
    CCSIDR     = <0xD80,rd,rd,NULL>,
    CCSELR     = <0xD84,rd,rd,wr>,
    
    # Cortex M33:
    DAUTHCTRL  = <0xE04,rd,rd,wr>,
    DSCSR      = <0xE08,rd,rd,wr>,
    
    # Cortex M7:
    ICIALLU    = <0xF50,NULL,NULL,wr>,
    ICIMVAU    = <0xF58,NULL,NULL,wr>,
    DCIMVAC    = <0xF5C,NULL,NULL,wr>,
    DCISW      = <0xF60,NULL,NULL,wr>,
    DCCMVAU    = <0xF64,NULL,NULL,wr>,
    DCCMVAC    = <0xF68,NULL,NULL,wr>,
    DCCISW     = <0xF6C,NULL,NULL,wr>,

    CM7_ITCMCR = <0xF90,rd,rd,wr>,
    CM7_DTCMCR = <0xF94,rd,rd,wr>,
    CM7_AHBPCR = <0xF98,rd,rd,wr>,
    CM7_CACR   = <0xF9C,rd,rd,wr>,
    CM7_AHBSCR = <0xFA0,rd,rd,wr>,
    CM7_ABFSR  = <0xFA8,rd,rd,wr>,

    DAUTHSTATUS = <0xFB8,rd,rd,wr>,

    # Cortex-M33 Non-secure register versions
    DFSR_NS  = <0x20D30,rd,rd,wr>,
    DHCSR_NS = <0x20DF0,rd,rd,wr>,
    DCRSR_NS = <0x20DF4,NULL,NULL,wr>,
    DCRDR_NS = <0x20DF8,rd,rd,wr>,
    DEMCR_NS = <0x20DFC,rd,rd,wr>,
    DAUTHCTRL_NS  = <0x20E04,rd,rd,wr>,
    DAUTHSTATUS_NS = <0x20FB8,rd,rd,wr>,
    ]
}

set fld.SCS [
    SHCSR     = [
        MEMFAULTACT    = <0,1,bitmsg>,
        BUSFAULTACT    = <1,1,bitmsg>,
        USGFAULTACT    = <3,1,bitmsg>,
        SVCALLACT      = <7,1,bitmsg>,
        MONITORACT     = <8,1,bitmsg>,
        PENDSVACT      = <10,1,bitmsg>,
        SYSTICKACT     = <11,1,bitmsg>,
        USGFAULTPENDED = <12,1,bitmsg>,
        MEMFAULTPENDED = <13,1,bitmsg>,
        BUSFAULTPENDED = <14,1,bitmsg>,
        SVCALLPENDED   = <15,1,bitmsg>,
        MEMFAULTENA    = <16,1,endismsg>,
        BUSFAULTENA    = <17,1,endismsg>,
        USGFAULTENA    = <18,1,endismsg>,
    ],
    DFSR      = [
        HALTED       = <0,1,bitmsg>,
        BKPT         = <1,1,bitmsg>,
        DWTTRAP      = <2,1,bitmsg>,
        VCATCH       = <3,1,bitmsg>,
        EXTERNAL     = <4,1,bitmsg>
    ],
    DHCSR      = [
        C_DEBUGEN    = <0,1,bitmsg>,
        C_HALT       = <1,1,bitmsg>,
        C_STEP       = <2,1,bitmsg>,
        C_MASKINTS   = <3,1,bitmsg>,
        C_SNAPSTALL  = <5,1,bitmsg>,
        S_REGRDY     = <16,1,bitmsg>,
        S_HALT       = <17,1,bitmsg>,
        S_SLEEP      = <18,1,bitmsg>,
        S_LOCKUP     = <19,1,bitmsg>,
        S_RETIRE_ST  = <24,1,bitmsg>,
        S_RESET_ST   = <25,1,bitmsg>,
    ],
    DEMCR      = [
        VC_CORERESET = <0,1,bitmsg>,
        VC_MMERR     = <4,1,bitmsg>,
        VC_NOCPERR   = <5,1,bitmsg>,
        VC_CHKERR    = <6,1,bitmsg>,
        VC_STATERR   = <7,1,bitmsg>,
        VC_BUSERR    = <8,1,bitmsg>,
        VC_INTERR    = <9,1,bitmsg>,
        VC_HARDERR   = <10,1,bitmsg>,
        MON_EN       = <16,1,bitmsg>,
        MON_PEND     = <17,1,bitmsg>,
        MON_STEP     = <18,1,bitmsg>,
        MON_REQ      = <19,1,bitmsg>,
        TRCENA       = <24,1,endismsg>,
    ],
    AIRCR      = [
        VECTRESET     = <1,1,bitmsg>,
        VECTCLRACTIVE = <1,1,bitmsg>,
        SYSRESETREQ   = <2,1,boolmsg "Don't request system reset"
                                     "Requesting system reset">,
        SYSRESETREQS  = <3,1,boolmsg "SYSRESETREQ available S- and NS-"
                                     "SYSRESETREQ available only in S-">,
        PRIGROUP      = <8,3,intmsg "subpri [%d:0], group have rest">,
        BFHFNMINS     = <13,1,boolmsg "bus, hard and NMI faults are secure"
                                      "bus fault and NMI are NS-">,
        PRIS          = <14,1,boolmsg "same priority for S- & NS- exceptions" "NS- exceptions de-prioritorized">,
        ENDIANNESS    = <15,1,boolmsg "little-endian" "big-endian">,
        VECTKEYSTAT   = <16,16,intmsg "0x%04X">,
    ],

    CM7_AHBPCR = [
        EN       = <0,1,bitmsg>,
        SZ       = <1,2,choicemsg "0MB - AHB disabled", "64MB", "256MB", "512MB"> 
    ],
    
]

# Data Watchpoint Trace registers for a Cortex-M33
set dev.DWT[rd,wr]:{[
    # Cortex M33:
    DWT_CTRL      = <0x000,rd,rd,wr>,
    DWT_CYCCNT    = <0x004,rd,rd,wr>,
    DWT_CPICNT    = <0x008,rd,rd,wr>,
    DWT_EXCCNT    = <0x00C,rd,rd,wr>,
    DWT_SLEEPCNT  = <0x010,rd,rd,wr>,
    DWT_LSUCNT    = <0x014,rd,rd,wr>,
    DWT_FOLDCNT   = <0x018,rd,rd,wr>,
    DWT_PCSR      = <0x01C,rd,rd,NULL>,
    DWT_COMP0     = <0x020,rd,rd,wr>,
    DWT_FUNCTION0 = <0x028,rd,rd,wr>,
    DWT_COMP1     = <0x030,rd,rd,wr>,
    DWT_FUNCTION1 = <0x038,rd,rd,wr>,
    DWT_COMP2     = <0x040,rd,rd,wr>,
    DWT_FUNCTION2 = <0x048,rd,rd,wr>,
    DWT_COMP3     = <0x050,rd,rd,wr>,
    DWT_FUNCTION3 = <0x058,rd,rd,wr>,
    DWT_LAR       = <0xFB0,rd,rd,wr>,
    DWT_LSR       = <0xFB4,rd,rd,wr>,
    ]
}


set local.fld_DWT_FUNCTION [
        MATCH        = <0,4,choicemsg <
                            "Disabled",
                            "CYCCNT Cycle Counter == COMPn",
                            "Instruction Address",
                            "Instruction Address in <COMP(n-1),COMPn>",
                            "Data Address in <COMPn,COMPn+DATAVSIZE-1>",
                            "Data Address write in <COMPn,COMPn+DATAVSIZE-1>",
                            "Data Address read in <COMPn,COMPn+DATAVSIZE-1>",
                            "Data Address in <COMP(n-1),COMPn>",
                            "Data Value == COMPn",
                            "Data Value written == COMPn",
                            "Data Value read == COMPn",
                            "Linked Data Value == COMPn when FUNCTION(n-1) matches",
                            "Data Address with Value == COMPn traced",
                            "Data Address with Value written == COMPn traced",
                            "Data Address with Value read == COMPn traced",
                            "Reserved"
                         >>,
        ACTION        = <4,2,choicemsg <
                            "Trigger only",
                            "Generate debug event",
                            "Generate data trace Match (or Value for tarce match) packet",
                            "Generate data trace Address, PC Value or Value packet"
                         >>,
        DATAVSIZE     = <10,2,choicemsg<"1 byte","2 bytes","4 bytes","<reserved>">>,
        MATCHED       = <24,1,bitmsg>,
        ID            = <27,5,[n]:{
             .msg = "<Illegal ID>";
             n==0x00 {msg = "Reserved"}!;
             n==0x08 {msg = "Data Address, and Data Address With Value."}!;
             n==0x09 {msg = "Cycle Counter, Data Address, and Data Address With Value."}!;
             n==0x0A {msg = "Instruction Address, Data Address, and Data Address With Value."}!;
             n==0x0B {msg = "Cycle Counter, Instruction Address, Data Address and Data Address With Value."}!;
             n==0x18 {msg = "Data Address, Data Address Limit, and Data Address With Value."}!;
             n==0x1A {msg = "Instruction Address, Instruction Address Limit, Data Address, Data Address Limit, and Data Address With Value."}!;
             n==0x1C {msg = "Data Address, Data Address Limit, Data Value, Linked Data Value, and Data Address With Value."}!;
             n==0x1E {msg = "Instruction Address, Instruction Address Limit, Data Address, Data Address Limit, Data value, Linked Data Value, and Data Address With Value."}!;
             msg
           }>,
]

set fld.DWT [
    # ARMv8-m
    DWT_CTRL      = [
        CYCCNTENA     = <0,1,endismsg>,
        POSTPRESET    = <1,4,intmsg "POSTCNT %d">,
        POSTINIT      = <5,4,intmsg "POSTCNT %d">,
        CYCTAP        = <9,1,boolmsg "tap at CYCCNT[6]" "tap at CYCCNT[10]">,
        SYNCTAP       = <10,2,choicemsg <"no sync packets",
                                         "tap at CYCCNT[24]",
                                         "tap at CYCCNT[26]",
                                         "tap at CYCCNT[28]">>,
        PCSAMPLENA    = <12,1,endismsg>,
        EXCTRCENA     = <16,1,endismsg>,
        CPIEVTENA     = <17,1,endismsg>,
        EXCEVTENA     = <18,1,endismsg>,
        SLEEPEVTENA   = <19,1,endismsg>,
        LSUEVTENA     = <20,1,endismsg>,
        FOLDEVTENA    = <21,1,endismsg>,
        CYCEVTENA     = <22,1,endismsg>,
        CYCDISS       = <23,1,endismsg>,
        NOPRFCNT      = <24,1,bitmsg>,
        NOCYCCNT      = <25,1,bitmsg>,
        NOEXITTRIG    = <26,1,bitmsg>,
        NOTRCPKT      = <27,1,bitmsg>,
        NUMCOMP       = <28,4,intmsg "%d comparators">,
    ],
    DWT_FUNCTION0 = local.fld_DWT_FUNCTION,
    DWT_FUNCTION1 = local.fld_DWT_FUNCTION,
    DWT_FUNCTION2 = local.fld_DWT_FUNCTION,
    DWT_FUNCTION3 = local.fld_DWT_FUNCTION,
    DWT_LSR = [
        SLI     = <0,1,boolmsg "not implemented" "implemented">,
        SLK     = <1,1,boolmsg "not locked" "locked">,
        nTT     = <1,1,bitmsg>,
    ],
 ]


# BPU Flash Patch regisers for Cortex-M33 
set dev.FBP[rd,wr]:{[
    # Cortex M3:
    FP_CTRL      = <0x000,rd,rd,wr>,
    FP_REMAP     = <0x004,rd,rd,wr>,
    FP_COMP0     = <0x008,rd,rd,wr>,
    FP_COMP1     = <0x00C,rd,rd,wr>,
    FP_COMP2     = <0x010,rd,rd,wr>,
    FP_COMP3     = <0x014,rd,rd,wr>,
    FP_COMP4     = <0x018,rd,rd,wr>,
    FP_COMP5     = <0x01C,rd,rd,wr>,
    FP_COMP6     = <0x020,rd,rd,wr>,
    FP_COMP7     = <0x024,rd,rd,wr>,
    ]
}


# Data Watchpoint Trace registers for a Cortex-M33
set dev.CTI[rd,wr]:{[
    # Cortex M33:
    CTICONTROL   = <0x000,rd,rd,wr>,
    CTIINTACK    = <0x010,NULL,NULL,wr>,
    CTIAPPSET    = <0x014,rd,rd,wr>,
    CTIAPPCLEAR  = <0x018,rd,rd,wr>,
    CTIAPPPULSE  = <0x01C,rd,rd,wr>,
    CTIINTEN0    = <0x020,rd,rd,wr>,
    CTIINTEN1    = <0x024,rd,rd,wr>,
    CTIINTEN2    = <0x028,rd,rd,wr>,
    CTIINTEN3    = <0x02C,rd,rd,wr>,
    CTIINTEN4    = <0x030,rd,rd,wr>,
    CTIINTEN5    = <0x034,rd,rd,wr>,
    CTIINTEN6    = <0x038,rd,rd,wr>,
    CTIINTEN7    = <0x03C,rd,rd,wr>,
    CTIOUTEN0    = <0x0A0,rd,rd,wr>,
    CTIOUTEN1    = <0x0A4,rd,rd,wr>,
    CTIOUTEN2    = <0x0A8,rd,rd,wr>,
    CTIOUTEN3    = <0x0AC,rd,rd,wr>,
    CTIOUTEN4    = <0x0B0,rd,rd,wr>,
    CTIOUTEN5    = <0x0B4,rd,rd,wr>,
    CTIOUTEN6    = <0x0B8,rd,rd,wr>,
    CTIOUTEN7    = <0x0BC,rd,rd,wr>,
    CTITRIGINSTATUS  = <0x130,rd,rd,NULL>,
    CTITRIGOUTSTATUS = <0x134,rd,rd,NULL>,
    CTICHINSTATUS = <0x138,rd,rd,NULL>,
    CTIGATE      = <0x140,rd,rd,wr>,
    ASICCTL      = <0x144,rd,rd,wr>,
    ITCHOUT      = <0xEE4,rd,rd,wr>,
    ITTRIGOUT    = <0xEE8,rd,rd,wr>,
    ITCHIN       = <0xEF4,rd,rd,wr>,
    ITCTRL       = <0xF00,rd,rd,wr>,
    ]
}



set bse [
    ITM = 0xE0000000,
    SCS = 0xE000E000,
    DWT = 0xE0001000,
    FBP = 0xE0002000,
    TPIU = 0xE0040000,
    ETM = 0xE0041000,
    CTI = 0xE0042000,
    SCS_NS = 0xE002E000,
]
