# LinkServer GDB server information

One of the functions of *LinkServer* is to launch and manage the GDB server instances.  
`./LinkServer gdbserver --help` command provides details about the `gdbserver` command.

A GDB server instance means a *crt_emu_cm_redlink* utility process started with the appropriate arguments.

The *crt_emu_cm_redlink* tool can be found in the `binaries` folder. One of its functions is to process [GDB remote protocol](https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html) requests. Also, it is possible to pass textual [GDB commands](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Commands.html#Commands) to it through GDB client and receive textual results via `monitor` command.

**Note**
 *crt_emu_cm_redlink* is not intended to be used as a standalone tool. But if it is needed, use `crt_emu_cm_redlink --help` command to see the supported arguments.

The LinkServer GDB server functions are targeted for debugging application running on Cortex-M cores, and can be used with standalone GDB clients or with GDB clients integrated within an IDE.
The GDB client is a tool that is part of [GNU Arm Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain).

The GDB commands to initialize a debug session are automatically issued by the IDE or can be provided manually when a standalone GDB client is used.

The [monitor commands](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html#Monitor-Commands-for-gdbserver-1) are implementation specific.

Here is the list of monitor commands supported by LinkServer GDB server:
* [help](#monitor-help)
* [capabilities](#monitor-capabilities)
* [catch](#monitor-catch)
* [counter](#monitor-counter)
* [flash_load_option](#monitor-flash_load_option)
* [info](#monitor-info)
* [kill_server](#monitor-kill_server)
* [ondisconnect](#monitor-ondisconnect)
* [register_display](#monitor-register_display)
* [register_modify](#monitor-register_modify)
* [reset ](#monitor-reset)
* [semihosting](#monitor-semihosting)
* [stdout](#monitor-stdout)

## monitor help

This will issue text explaining the supported monitor commands and their syntax.

Example:
```
(gdb) monitor help 
capabilities                        -- display capabilities string
catch [set=, | clear=] [event-list | all]
                                    -- set or clear event-catches
counter [watch=0x1234 | clear | set=0x0]
                                    -- watch/clear/set/reset/show cycle counter
flash_load_option [mass_at_0 | no_mass_at_0 | mass_not_0 | no_mass_not_0 | check_0 | no_check_0 | default]
                                    -- set erase rules for load into flash.
help [command]                      -- show info on all or a specific monitor command
info [,all | ,emu | ,periph ]       -- display target information
kill_server                         -- causes the GDB server to quit when GDB client does
ondisconnect [nochange | stop | cont | run_cont]
                                    -- behavior on disconnect/quit
register_display [* | -| core[-extra|-all] | <name>[!][/] ]
                  *                 -- display all peripheral names
                  -                 -- display all peripheral details
                  core              -- display main core registers
                  core-extra        -- display additional core registers
                  core-all          -- display main+additional registers
                  <name>[!][/]      -- display peripheral, reg, or field
register_modify [$]<name>=<value>   -- update peripheral or register
reset                               -- issue a processor connection reset
semihosting [ena | dis]             -- enable or disable semihosting
stdout [buffered | unbuffered]      -- buffer or unbuffer stdout
(gdb)
```

If given the name of another monitor command it will print help about that command. Example:
```
(gdb) monitor help semihosting
Semihosting allows a target application to access host services
such as files, and notably stdio (printf). It does this in a
very non-realtime way (breakpoints). It also requires software
breakpoints on some systems. The default depends on whether
software breakpoints are builtin to the processor.
(gdb)
```

## monitor capabilities

Displays capabilities using upper/lower case letters.

Example:
```
(gdb) monitor capabilities
CRTv00.1/Apfe/MB08W04C1S/rt/Lf
(gdb)
```

## monitor catch

Usage: `catch [set=, | clear=] [event-list | all]`

Set, Clear, or Display event catches.

Used without any arguments, this will display all catchable events by name, and current state. Otherwise, you can set or clear one or more by a comma separated list (or 'all').

Example:
```
(gdb) monitor catch set=reset
```

## monitor counter

Usage: `counter [watch=0x1234 | clear | set=0x0]`

Catch or Clear, (re)Set, or show the state of the cycle counter.
- Watch sets the value to match. When the cycle counter reaches that number, it will stop execution.
- Clear removes the watch.
- Set is used to set a new value for the counter, which is reset by load.
Note that the 'FPS' register shows the current counter number as a relative value (from last execution start).

Example:
```
(gdb) monitor catch set=reset
```

## monitor flash_load_option

Usage: `flash_load_option [mass_at_0 | no_mass_at_0 | mass_not_0 | no_mass_not_0 | check_0 | no_check_0 | default]`

Set erase rules for load into flash.

Choose one of
- mass_at_0,
- no_mass_at_0, or
- default (mass at 0).

Choose one of
- mass_not_0,
- no_mass_not_0, or
- default (no mass on not 0)

Choose one of
- check_0,
- no_check_0, or
- default.

Default is dependent on the target processor. Check_0 means to check reset vector when not loading at 0.

## monitor info

Usage: `info [,all | ,emu | ,periph]`

Show information on the target and/or emulator.

This command's output will be different depending on whether the GDB server was started with -mi or not (MI is used by Eclipse). If started with -mi, output will be XML, else normal text.
- The ',all' qualifier causes all information to be displayed (same as no qualifier).
- The ',periph' qualifier only shows the peripheral information.
- The ',emu' qualifier only shows the emulator defaults.

## monitor kill_server

Causes the GDB server to quit when GDB does.

Example:
```
(gdb) monitor kill_server
Server will now quit when GDB does
```

## monitor ondisconnect

Usage: `ondisconnect [nochange | stop | cont | run_cont]`

When disconnecting from the target (ending a debug session), it is possible to choose what state the target is left in.

The default is the same as when disconnecting. It is possible always to have it stop, always to have it continue, and to run (reset) and continue.

Example:
```
(gdb) monitor ondisconnect
Current on disconnect action is: cont

(gdb) monitor ondisconnect stop
(gdb) monitor ondisconnect
Current on disconnect action is: stop

(gdb) monitor ondisconnect cont
(gdb) monitor ondisconnect
Current on disconnect action is: cont

(gdb)
```

## monitor reset

Issues a specific reset, based on the target and image type.
The reset method (hard/system/core/soft/romstall/reset script) depends on
the target reset configuration and/or the image type (flash/RAM).

Example:
```
(gdb) monitor reset
Processor connection reset
(gdb)
```

## monitor register_display

Usage: `register_display [ core | core-extra | core-all |  <register-name>[!][/] ]`

Display information about either peripherals or registers.

### Command use - `register_display core | core-extra | core-all`

Display a set of registers as a set of register categories with each category preceded with the name of the category.

Different sets of registers are displayed depending on the argument
- core - the main set of processor registers 
- core-extra  - an additional set of registers of interest (currently this seems to provide the same registers as 'core')
- core-all - both the core and core-extra registers together

Register category names (e.g.  'Core Registers', '<ch>IntMask', '', '<ch>FP Registers') appear in square brackets optionally on a line of their own.  If the category name is preceded by a '+' or '-' it indicates that the registers that follow are enabled (+) or disabled (-).  For example if the 'IntMask' category is listed as '+IntMask' the following interrupt registers are enabled, otherwise interrupts have been disabled. The '' category (which appears as '[]') introduces a set of pseudo-registers that are constructed by the GDB server. 

These are followed by lines each describing a given register:  
`[+|-|!] [{<gdbname>}] <name> = <value> [: <valuedescription>]`

These lines optionally begin with a '+', '-' or '!' character
- no character - register is readable and writeable
- '+' - register is write only
- '-' - register is read only
- '!' - not really a register at all, a psuedo register

This is then followed optionally by the name this register is referred to in GDB in braces.

The GDB server's name for the register follows and then an '=' sign.

The value that follows is normally presented as a hexadecimal number, but if it can not be read it may appear as '<error>', or if it logically has no associated value it may appear as 'disabled'.

If this is followed by a colon (:) the remainder of the line is text that provides further information about the meaning of the value intended for the user.

For example:  `register_display core` might produce this output:
```
(gdb) mon register_display core
[Core Registers]
{r0}R0=0x000003E8
{r1}R1=0x00400000
{r2}R2=0x2000001C
{r3}R3=0x2000001C
{r4}R4=0x000001D4
{r5}R5=0x00000000
{r6}R6=0x00000000
{r7}R7=0x2002FFE8
{r8}R8=0x00000000
{r9}R9=0x00000000
{r10}R10=0x00000000
{r11}R11=0x00000000
{r12}R12=0x00000000
{sp}SP=0x2002FFE8
{lr}LR=0x0000098B
{pc}PC=0x00000942
{cpsr}xPSR=0x21000000
!Flags=0x21000000:nzCvq
!EPSR=0x21000000:none
!IPSR=0x21000000:0  (Base)
!MSP=0x2002FFE8
PSP=0x00000000
CONTROL=0x0
[+IntMask]
BasePri=0x00
PriMask=0x00
FaultMask=0x00
[]
!Cycle=0x00000111
-CycleDelta=0x1409B6E0
!VECTPC=<error>
!CFSR=0x00000000
!Faults=none
!MMAR=ignore
!BFAR=ignore
[]
[+FP Registers]
{fpscr}FPSCR=0x00000000
{fpccr}FPCCR=0xC0000000
{fpcar}FPCAR=0x00000000
{fpdscr}FPDSCR=0x00000000
{mvfr0}MVFR0=0x10110021
{mvfr1}MVFR1=0x11000011
{s0}S0=0x00000000
{s1}S1=0x00000000
{s2}S2=0x00000000
{s3}S3=0x00000000
{s4}S4=0x00000000
{s5}S5=0x00000000
{s6}S6=0x00000000
{s7}S7=0x00000000
{s8}S8=0x00000000
{s9}S9=0x00000000
{s10}S10=0x00000000
{s11}S11=0x00000000
{s12}S12=0x00000000
{s13}S13=0x00000000
{s14}S14=0x00000000
{s15}S15=0x00000000
{s16}S16=0x00000000
{s17}S17=0x00000000
{s18}S18=0x00000000
{s19}S19=0x00000000
{s20}S20=0x00000000
{s21}S21=0x00000000
{s22}S22=0x00000000
{s23}S23=0x00000000
{s24}S24=0x00000000
{s25}S25=0x00000000
{s26}S26=0x00000000
{s27}S27=0x00000000
{s28}S28=0x00000000
{s29}S29=0x00000000
{s30}S30=0x00000000
{s31}S31=0x00000000
{d0}D0=0x0000000000000000
{d1}D1=0x0000000000000000
{d2}D2=0x0000000000000000
{d3}D3=0x0000000000000000
{d4}D4=0x0000000000000000
{d5}D5=0x0000000000000000
{d6}D6=0x0000000000000000
{d7}D7=0x0000000000000000
{d8}D8=0x0000000000000000
{d9}D9=0x0000000000000000
{d10}D10=0x0000000000000000
{d11}D11=0x0000000000000000
{d12}D12=0x0000000000000000
{d13}D13=0x0000000000000000
{d14}D14=0x0000000000000000
{d15}D15=0x0000000000000000
```

### Command use - `register_display <name>[!][/]`

The name provided can be a core register name.

Currently it will also accept
- the name of a peripheral
- the name of a register within any peripheral (the first with a register of  that name is chosen)
 - `<peripheral>.<register>` - the name of a peripheral and a register within it
- `<peripheral>.<register>.<field>` - the name of a field within a register within a peripheral
 - `<register>.<field>` - the name of a field within any peripheral (the first with a register of that name is chosen).

which will display peripheral information. ***But this usage is deprecated and will be removed***.

The names do not have to agree in case with the names listed in register_display core (which are typically uppercase).

These will display details of a given register.

Two letters can follow the name of the resister or peripheral to be displayed
- ! - means that volatile registers will be read as well as non-volatile ones
- / - means that field contents will also be displayed

Lines showing register contents are either  
`[?] <name> [=|:] <value>`

These lines optionally begin with a '?' which is present if the register is volatile (i.e. it may alter if read).

An equal sign (=) follows the name normally, but this is replaced by a colon (:) if the register is read-only,

The value is displayed as '?' if the register is volatile and volatile reads have not been requested, or as "error" if the value could not be read, or otherwise either in decimal or hexadecimal or may be the corresponding enumeration value name for an enumerated value.

Lines showing field contents contain '!" before the register name and are on lines immediately following the display of the parent register.  These are presented on lines with the syntax `[?]!<name>=<value>`

where the value is provided either as a hexadecimal value or as a text string interpreting its meaning, intended for a user.  As with the register content an initial '?' is used to indicate that the value of the field is volatile.

Example core register from register_display r1
```
r1=0x00000000
```

# monitor register_modify

Usage: `register_modify [$]<name>=<value>`

Write a value to the given register.

The name provided can be a core register name.

A dollar sign ($) may precede the register name but it is ignored.

Currently it will also accept
- the name of a register within any peripheral (the first with a register of that name is chosen)
- `<peripheral>.<register>` - the name of a peripheral and a register within it

which will modify peripheral register. ***But this usage is deprecated and will be removed.***

## monitor semihosting

Usage: `monitor semihosting [ena | dis]`

With no argument reports whether semihosting is enabled or disabled. Otherwise
- ena - enables semihosting
- dis - disables semihosting

When semihosting is disabled the semihosting BKPT instructions executed on the target are ignored (by continuing execution on the target) otherwise the accompanying I/O request is decoded and acted upon.

Example:
```
(gdb) monitor semihosting
Semihosting enabled.

(gdb) monitor semihosting ena
Semihosting enabled.

(gdb) monitor semihosting dis
Semihosting disabled.
(gdb)
```

## monitor stdout

Usage: `stdout [buffered | unbuffered]`

If semihosted I/O is in use this command controls whether target semihosted output is buffered.
If no argument is provided the command reports whether semihosted output is buffered or not. Otherwise
 - buffered - the output is set to be buffered
 - unbuffered - the output is set to be unbuffered

When output is unbuffered each write by the host is written directly to a the semihosted output channel.When it is buffered it is collected in a buffer which is then written to the semihosted output channel every few microseconds (if there is some data).
