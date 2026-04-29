# LinkServer scripting support
The *LinkServer* debug server supports a Basic like programming language that can be used to script [low level target operations](LinkServer-LowLevelFunctions.md).

## Script format
*LinkServer* scripts are written in a simple version of the BASIC programming language. In this variant of BASIC, **26 variables** are available (%a through %z).

They offer functionality as shown in the [LinkServer low level functions](LinkServer-LowLevelFunctions.md) section.

Arguments can be passed in to the script by assigning values to the variables, but the variables that can be used depend on the method of script invocation. Scripts on the *LinkServer* can be invoked by:
- via telnet and console connections: by assigning values to the variables explicitly before causing the script text to run
- on the Remote Procedure Call connections (via GDB server): by providing values for four RPC function values which are always assigned to a%, b%, c% and d%

## Supplied scripts
A set of scripts (*.scp) are supplied within the *LinkServer* installation at:
 - [binaries/Scripts](../binaries/Scripts/)
 - [binaries/ToolScripts](../binaries/ToolScripts/)

These scripts can be referenced from device specific JSON data files, when needed.
The call outs where scripts can be referenced (if required) are:
- `preconnect-script` and `connect-script` are intended to assist with the initial debug connection
- `preattach-script` is intended to assist with the attach connection
- `reset-script` is intended to assist with the target reset
- `masserase-script` is intended to assist with the resurrect of a locked Kinetis/MCXC/MCXE24x device

### Preconnect scripts
Some chips require a preconnect script that prepares the target MCU for the initial debug connection.
The purpose of a `preconnect-script` (if present) is to provide any additional help necessary in gaining access to the debug infrastructure (e.g. the Coresight DAP) prior to any debug session being requested.
The execution of these scripts are requested through the telnet interface to the LinkServer.
On entry to the `preconnect-script` these variables have these values assigned:
- a% is the probe index used

### Preattach scripts
A preattach script is similar to a preconnect script, but it is intended to assist with the attach connection.
The purpose of a `preattach-script` (if present) is to provide a method to do custom target initializations prior to the attach session.
The execution of these scripts are requested through the telnet interface to the LinkServer.
On entry to the `preattach-script` these variables have these values assigned:
- a% is the probe index used

### Connect scripts
The purpose of a `connect-script` is to provide any additional help necessary to gain access to the specified debug bus at the beginning of a debug session.
The execution of these scripts is requested by the GDB server through the RPC interface to the LinkServer.
On entry to the connect script these variables have these values assigned:
- a% is the probe index used
- b% is the core index
- No use is made of any of the variables on exit.

### Reset scripts
The purpose of a `reset-script` is to provide a means to cause code held in Flash to execute whilst eliminating the legacy of any pre-existing chip state.
The execution of these scripts is requested by the GDB server through the RPC interface to the LinkServer.
A `reset-script` overrides the default debug reset behaviour. A `reset-script` is less commonly required than a `connect-script` but can be used to work around issues where a standard reset may not allow debug operations to survive.
On entry to a `reset-script` some variable have assigned values:
- a% is the PC
- b% is the SP
- c% is the XPSR
- d% is the VTOR
On exit from the script %a is loaded into the PC and %b is loaded into the SP, thus providing a way for the script to change the startup behavior of the application.

### Pre-created scripts
The purpose of certain scripts is described below:
- [kinetismasserase.scp](../binaries/Scripts/kinetismasserase.scp) - invoked by the Flash Programmer to resurrect locked Kinetis device
- [kinetisunlock.scp](../binaries/Scripts/kinetisunlock.scp) - if for any reason the Flash Programmer fails to resurrect a locked part (as above), this script can be specified in place of the above and the recovery attempt repeated
- [delayexample.scp](../binaries/Scripts/delayexample.scp) - an example script showing how a delay can be performed

On occasion it can be useful to run and debug code directly from RAM. Since an MCU will not boot from RAM a scheme is needed to take control of the debuggers reset mechanism. This can be achieved by the use of a LinkServer reset script.
Within *LinkServer* installation, certain pre-created scripts are located at [binaries/Scripts](../binaries/Scripts/).

Contained in this directory is a script called [kinetisRamReset.scp](../binaries/Scripts/kinetisRamReset.scp) (see below).
```
1 REM ======================================
2 REM Copyright 2020, 2023 NXP
3 REM All rights reserved.
4 REM SPDX-License-Identifier: BSD-3-Clause
5 REM ======================================

10  REM Kinetis K64F Internal RAM (@ 0x20000000) reset script
20  REM Connect script is passed PC/SP from the vector table in the image by the debugger
30  REM For the simple use case we pass them back to the debugger with the location of the reset context.
40  REM
50  REM Syntax here is that '~' commands a hex output, all integer variables are a% to z%
70  REM Find the probe index
80  p% = probefirstfound
90  REM Set the 'this' probe and core
100 selectprobecore p% 0
110 REM NOTE!! Vector table presumed RAM location is address 0x20000000
120 REM The script passes the SP (%b) and PC (%a) back to the debugger as the reset context.
130 b% = peek32 this 0x20000000
140 a% = peek32 this 0x20000004
145 d% = 0x20000000
150 print "Vector table SP/PC is the reset context."
160 print "PC = "; ~a%
170 print "SP = "; ~b%
180 print "XPSR = "; ~c%
185 print "VTOR = "; ~d%
190 end
```
This reset script makes an assumption that the user intends to run code from RAM at 0x20000000 - this is the value of the SRAM_Upper RAM block on Kinetis parts.

## User scripts
Additional user generated scripts can be added directly to the LinkServer product installation, at [binaries/Scripts](../binaries/Scripts/) or [binaries/ToolScripts](../binaries/ToolScripts/). These user scripts can be referenced from custom JSON data files, when needed.

