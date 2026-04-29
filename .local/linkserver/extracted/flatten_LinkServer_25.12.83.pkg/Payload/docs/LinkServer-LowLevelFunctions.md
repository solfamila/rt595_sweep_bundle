# LinkServer low level functions
*LinkServer* is using the *redlinkserv* low level tool to communicate with a debug probe.
The tool can be found in the `binaries` folder.

The functions supported by *redlinkserv* can be accessed as follows:
- directly, in command line mode: `binaries/redlinkserv --commandline`
- indirectly, via a client connected to the telnet port specified at GDB server launch time:
    - `./LinkServer gdbserver --redlink-telnet-port INTEGER [other OPTIONS] DEVICE`
- indirectly, from Basic like scripts which can be specified within a target JSON data file to be run before a connection and/or before a reset. Refer to [LinkServer scripting support](LinkServer-ScriptingSupport.md) for details.

## Probe related functions
```
PROBELIST : Enumerates and returns an indexed list of known probe types
PROBENUM : Returns the number of probes attached
PROBEOPENBYINDEX <ProbeIndex> [<"FILENAME">] : Opens the probe associated with ProbeIndex
   FILENAME is text of <key = value> pairs used for internal configuration
PROBEOPENBYSERIAL <"SerialNumber"> : Opens the probe associated with SerialNumber
PROBECLOSEBYINDEX <ProbeIndex> : Closes the probe associated with ProbeIndex
PROBECLOSEBYSERIAL <"SerialNumber"> : Closes the probe associated with SerialNumber
PROBEFIRSTFOUND : Returns the THIS ProbeIndex or index of the first probe in the enumerated list
PROBETIME <ProbeIndex> : Returns elapsed time from firmware boot, if supported
PROBESTATUS [<ProbeIndex>]: Returns an indexed list summary of the status of the probes connected to the system
PROBEVERSION <ProbeIndex>: Returns CMSIS-DAP version information
PROBEDAPINFO <ProbeIndex>: Returns CMSIS-DAP probe information
PROBEISOPEN <ProbeIndex>: Returns TRUE or FALSE
PROBEHASJTAG <ProbeIndex>: Returns TRUE or FALSE
PROBEHASSWD <ProbeIndex>: Returns TRUE or FALSE
PROBEHASSWV <ProbeIndex>: Returns TRUE or FALSE
PROBEHASETM <ProbeIndex>: Returns TRUE or FALSE
PROBERESET <ProbeIndex> <ResetType>: Resets the probe (use 1 for ISP reset) [MCU-Link only]
```

## Core/TAP related functions
```
CORECONFIG {[THIS] | [<ProbeIndex>]}: Queries the scan chain configuration
CORESCONFIGURED <ProbeIndex>: Returns TRUE or FALSE
APLIMIT {[THIS] | [<ProbeIndex>]}: <APIndex>: Limit the AP Query (set once)
APLIST {[THIS] | [<ProbeIndex>]}: [<APLimit>]: Detailed list of APs connected to the specified probe. APLimit restricts queries to AP index.
CORELIST {[THIS] | [<ProbeIndex>]}: [<APLimit>]: Detailed list of APs/Cores connected to the specified probe. APLimit restricts queries to AP index.
COREREADID {[THIS] | [<ProbeIndex> <CoreIndex>]}: Returns the DpID
DEBUGMAILBOXREQ {[THIS] | [<ProbeIndex> <APIndex>]} <Request>: Debug Mailbox Request
```

## Wire related functions
```
WIRESWDCONNECT {[THIS] | [<ProbeIndex>]}: Configures the wire for SWD and returns the DpID
WIREJTAGCONNECT {[THIS] | [<ProbeIndex>]}: Configures the wire for JTAG
WIREDISCONNECT {[THIS] | [<ProbeIndex>]}: Closes the wire connection (SWD/JTAG)
WIREISPRESET {[THIS] | [<ProbeIndex>]}: Resets an LPC part into the ISP bootloader
WIREBOOTCONFIGSET {[THIS] | [<ProbeIndex>]} <"DATA">: Stores boot configuration data that will be automatically applied during subsequent reset commands.
    DATA is a string with up to 4 characters describing how each ISP_CTRL[3..0] pin should be handled:
    '0' (= drive low), '1' (= drive high), 'x' (= do not drive)
WIREBOOTCONFIGGET {[THIS] | [<ProbeIndex>]}: Returns previously stored configuration data
WIREBOOTCONFIGREAD {[THIS] | [<ProbeIndex>]}: Returns the current state of ISP_CTRL[3:0] pins
WIREBOOTCONFIGAPPLY {[THIS] | [<ProbeIndex>]} <1/0>: Immediately starts/stops driving the ISP_CTRL pins based on previously stored boot configuration data
WIRETIMEDRESET <ProbeIndex> <ms>: Asserts (Low) reset for ms milliseconds and returns the end state of the wire
WIREHOLDRESET <ProbeIndex> <State> : Asserts/Releases (Low/High) reset and returns the end state of the wire
WIRESETSPEED <ProbeIndex> <Hz>: Requests a particular wire speed in Hz
WIREGETSPEED <ProbeIndex> : Returns the current wire speed
WIRESETIDLECYCLES <ProbeIndex> <Cycles>: Sets the number of idle cycles between debug transactions
WIREGETIDLECYCLES <ProbeIndex> : Returns the current number of debug idle cycles
WIREISCONNECTED <ProbeIndex>: >: Returns TRUE or FALSE if WIRESWDCONNECT or WIREJTAGCONNECT is complete
WIREGETPROTOCOL <ProbeIndex>: Returns SWD or JTAG
SELECTPROBECORE <ProbeIndex> <CoreIndex> : Sets the THIS parameter Probe/Core pair
THIS : Displays the current Probe, Core pair
```

## Cortex-M related functions
```
CMINITAPDP {[THIS] | [<ProbeIndex> <CoreIndex>]}: Initialize a CMx core ready for debug connections
CMUNINITAPDP {[THIS] | [<ProbeIndex> <CoreIndex>]}: UnInitialize a CMx core (de-assert debug and system power-up)
CMWRITEDP {[THIS] | [<ProbeIndex> <CoreIndex>]} <REG> <DATA>: Returns zero on success
CMWRITEAP {[THIS] | [<ProbeIndex> <CoreIndex>]} <REG> <DATA>: Returns zero on success
CMREADDP {[THIS] | [<ProbeIndex> <CoreIndex>]} <REG>: Returns data
CMREADAP {[THIS] | [<ProbeIndex> <CoreIndex>]} <REG>: Returns data (handles RDBUF on AP reads)
CMCLEARERRORS {[THIS] | [<ProbeIndex> <CoreIndex>]}
CMHALT {[THIS] | [<ProbeIndex> <CoreIndex>]}
CMRUN {[THIS] | [<ProbeIndex> <CoreIndex>]}
CMSTEP {[THIS] | [<ProbeIndex> <CoreIndex>]}
CMREGS {[THIS] | [<ProbeIndex> <CoreIndex>]}
CMDEBUGSTATUS {[THIS] | [<ProbeIndex> <CoreIndex>]}
CMWRITEREG {[THIS] | [<ProbeIndex> <CoreIndex>]} <RegNumber> <Value>
CMREADREG {[THIS] | [<ProbeIndex> <CoreIndex>]} <RegNumber>
CMWATCHLIST {[THIS] | [<ProbeIndex> <CoreIndex>]}
CMWATCHSET {[THIS] | [<ProbeIndex> <CoreIndex>]} <DWTIndex> <Address> [<RW|R|W>]
CMWATCHCLEAR {[THIS] | [<ProbeIndex> <CoreIndex>]} <DWTIndex>
CMBREAKLIST {[THIS] | [<ProbeIndex> <CoreIndex>]} : List the FPB breakpoints
CMBREAKSET {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address> : Set an FPB
CMBREAKCLEAR {[THIS] | [<ProbeIndex> <CoreIndex>]} [<Address>] : Clear an FPB
CMSYSRESETREQ {[THIS] | [<ProbeIndex> <CoreIndex>]} : System reset request
CMVECTRESETREQ {[THIS] | [<ProbeIndex> <CoreIndex>]} : Core reset request
CMRESETVECTORCATCHSET {[THIS] | [<ProbeIndex> <CoreIndex>]} : Enable reset vector catch
CMRESETVECTORCATCHCLEAR {[THIS] | [<ProbeIndex> <CoreIndex>]} : Disable reset vector catch
```

## Generic BASIC like functions
```
PEEK8  {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address>
PEEK16 {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address>
PEEK32 {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address>
POKE8  {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address> <Data>
POKE16 {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address> <Data>
POKE32 {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address> <Data>
QPOKE8  {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address> <Data>
QPOKE16 {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address> <Data>
QPOKE32 {[THIS] | [<ProbeIndex> <CoreIndex>]} <Address> <Data>
QSTARTTRANSFERS {[THIS] | [<ProbeIndex> <CoreIndex>]} <NumReads>
MEMDUMP {[THIS] | [<ProbeIndex> <CoreIndex>]} <Byte Address> <Length>
MEMLOAD {[THIS] | [<ProbeIndex> <CoreIndex>]} <FileName> <Byte Address> <Length Limit> Loads binary file data to memory
MEMSAVE {[THIS] | [<ProbeIndex> <CoreIndex>]} <FileName> <Byte Address> <Length> Saves memory to binary file
EXIT: Exit the server
PRINT "TEXT"[;[~]Variable | Constant]: Print statement. Prints quoted text
  and/or value of an internal variable (a%% - z%%), or constant integer
  expression in decimal, or hexadecimal[~] format
TIME : Returns an incrementing centisecond count from the host
TIMEMS : Returns an incrementing millisecond count from the host
WAIT <msec> : Wait for the number of milliseconds before proceding
LIST: Lists a loaded script
NEW: Erases a loaded script from memory
RENUMBER <Delta>: Renumber script lines with Delta increment (default is 10)
LOAD <"FILENAME">: Loads a script from the current, absolute, or relative directory
SAVE <"FILENAME">: Saves a script to the current, absolute, or relative directory
```

## Generic BASIC like functions that only work inside scripts
```
GOTO <LineNumber>
IF <relation> THEN <statement> [ELSE <statement>]
REPEAT : Start of a repeat block
UNTIL <relation> : End with condition of repeat block
BREAKREPEATTO <LineNumber> : Premature end of a repeat loop
GOSUB <LineNumber>
RETURN
```

## Miscelanous
```
HELP : display help on LinkServer commands
VERSION : returns the LinkServer version
CONNECTIONS : display active connections
```