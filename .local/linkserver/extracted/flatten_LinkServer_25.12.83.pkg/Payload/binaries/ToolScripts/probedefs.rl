# Copyright 2020, 2022 NXP

#-----------------------------------------------------------------------------
# Debug probe definitions for rltool
#-----------------------------------------------------------------------------


# This file must create probe definitions in the 'probedef' directory object
# These are the pobes that can be selected using --probetype
# The list is displayed to users if '--probetype help' is used


# These properties are required in each definition
#  name    - a printable name to use to describe this probe type
#  clock   - the probe SWO UART's base clock from which its other baud rates
#            can be derived, or 0 to signify that any clock rate can be
#            requested
#  maxdiv  - the maximum clock divider used to derive clock rates
#  maxbaud - the maximum clock rate that can be requested
#  backend - the name of the backend (see "domain spooler") to be used
#  streamport[self,spec]
#          - a function returning a port name given a text specification
#            self - is the probe definition (defined here)
#            spec - is a string usually of the form [<serial>][:<n>] which
#                   identifies the probe as the <n>th that is enumerated
#                   (with the <serial> number, if given)
#            importantly, "<serial>" finds the first probe with serial number
#            <serial>, and ":1" finds the 'first' probe (most useful when there
#            is only one)
# further properties can be added (e.g. in order to provide for the streamport
# function)

#-----------------------------------------------------------------------------
# Common backend properties
#-----------------------------------------------------------------------------


# common properties for probes supported by the linkserv backend
set backend_linkserv [
    backend = "linkserv",
    streamport = [id,spec]:{strf "/%s" <spec>!}
]

# common properties for probes supported by the nxphid backend
set backend_nxphid [
    backend = "nxphid",
    streamport = [self,spec]:{
        strf "/0x%X:0x%X:0x%X/%s" <self.hidinfo.0,self.hidinfo.1,self.hidinfo.2,
                                   if spec==NULL{":1"}{spec}!>!
    }
]



#-----------------------------------------------------------------------------
# Debug probe definitions
#-----------------------------------------------------------------------------


# a Link2 probe is based on an LPC43xx normally clocked at 156MHz used with
# SWO from CMSIS-DAP from redlinkserv
set probedef.link2 backend_linkserv::[
    name = "Link2",
    clock = 156000000,  # UART base clock in Hz
    maxdiv = 0xFFFF,    # the max clock divisor we can use for the LPC43xx UART
    maxbaud = 10000000, # the max baud rate the probe's UART can be run at
]


# a Link2 probe is based on an LPC43xx normally clocked at 156MHz used with
# SWO directly from NXP USB HID
set probedef.link2_hid backend_nxphid::[
    name = "Link2 (NXP HID)",
    clock = 156000000,  # UART base clock in Hz
    maxdiv = 0xFFFF,    # the max clock divisor we can use for the LPC43xx UART
    maxbaud = 10000000, # the max baud rate the probe's UART can be run at
    hidinfo=<0x1fc9, 0x0090, 0xffeb>,
]


# a non-bridged (CMSIS-DAP only) Link2 probe is based on an LPC43xx normally
# clocked at 204MHz
set probedef.link2_nb backend_linkserv::[
    name = "Link2-NB",
    clock = 204000000,  # UART base clock in Hz
    maxdiv = 0xFFFF,    # the max clock divisor we can use for the LPC43xx UART
    maxbaud = 10000000, # the max baud rate the probe's UART can be run at
]


# an older Link2 probe is based on an LPC43xx normally clocked at 144MHz
set probedef.link2_144 backend_linkserv::[
    name = "Link2-144",
    clock = 144000000,  # UART base clock in Hz
    maxdiv = 0xFFFF,    # the max clock divisor we can use for the LPC43xx UART
    maxbaud = 10000000, # the max baud rate the probe's UART can be run at
]


# a ULINKPlus probe is based on an LPC43xx normally clocked at 204MHz
set probedef.ulinkplus backend_linkserv::[
    name = "uLink+",
    clock = 0,  # UART can set any baud rate ... UART base clock irrelevant
    maxdiv = 0xFFFF,    # the max clock divisor we can use for the LPC43xx UART
    maxbaud = 50000000, # the max baud rate the probe's UART can be run at
]


# a MCU-Link probe is based on an LPC55S69 normally clocked at 96MHz
set probedef.mculink backend_linkserv::[
    name = "MCU-Link",
    clock = 0, # 44000000,  #  UART can set any baud rate 
    maxdiv = 0xFFFF,    # the max clock divisor we can use for the LPC43xx UART
    maxbaud = 9600000,  # the max baud rate the probe's UART can be run at
]


set probedef.mculink_hid backend_nxphid::[
    name = "MCU-Link (NXP HID)",
    clock = 0, # 44000000,  #  UART can set any baud rate 
    maxdiv = 0xFFFF,    # the max clock divisor we can use for the LPC43xx UART
    maxbaud = 9600000,  # the max baud rate the probe's UART can be run at
    hidinfo=<0x1fc9, 0x0143, 0xffeb>,
]
