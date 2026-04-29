# LinkServer miscellaneous configurations

The `LinkServer` debug support for a particular board or device is based on the information included into the corresponding JSON file(s). A JSON file can include one or more entries.
For example the support for several legacy devices is included in [preinstalled_devices.json](..\devices\preinstalled_devices.json).

## Device specific configuration
Besides the device identification information, the `device` configuration includes properties related to cores and memory regions (especially the flash regions and the associated flash drivers).

## Debug specific configuration
The `debug` properties that can be configured for a device are listed below:
- `core-options:<core-name>:cachelib`: specific cache library used by LinkServer to manage memory accesses during the debug sessions (`--cachelib <lib>.so` miscellaneous option for LinkServer GDB server)
- `connect-reset`: reset method on initial connection (`system`, `core`, `default`)
- `flash-driver-reset`: reset method before running the RAM-loaded flash driver (`system`, `core`, `soft`)
- `reset`: reset method used to start the image loaded (`system`, `core`, `soft`)
- `preattach-script`: a pre-attach script used for attach sessions
- `preconnect-script`: a pre-connect script used for debug sessions
- `connect-script`: a connect script used for debug sessions
- `reset-script`: a reset script used for debug sessions
- `masserase-script`: a mass erase script used for resurrecting some devices
- `bootromstall`: BootROM stall address
- `romstalldelay`: BootROM stall delay in milliseconds
- `no-flash-hashing`: disable flash hashing (enabled by default)
- `no-packed`: Debug Access Port (DAP) does not support packed transfers
- `dapstride`: DAP stride mode (`auto`, `standard`, `long`, `short`)
- `apindex-limit`: DAP AP index limit
- `protocol`: debug protocol (`swd`, `jtag`)
- `wirespeed`: wire speed in Hz
- `swo`: has SWO support
- `bootconfigs`: boot configurations (`name:<data>` pairs) aimed to be used with MCU-Link on-board probes. `<data>` is a string of 4 characters ('x', '1' or '0') describing how each ISP_CTRL[3..0] pin should be handled.

### Cache library (`core-options:<core-name>:cachelib`)

In case of applications that enable caching, the LinkServer GDB server needs to make use of an appropriate `cachelib` for cache management, otherwise writes from the debugger would not be visible to the core (for example the setting software breakpoints).

The cache libraries currently supported are:
- `libahb_lpcac.so`: specific to AHB LPCAC controller and Cortex-M4 core (supported on some older devices such as K32L3A)
- `libahb_lmem.so`: specific to AHB LMEM controller and Cortex-M4 core (RT116x/RT117x devices)
- `libahb_xcache.so`: specific to XCACHE controler and Cortex-M33 core (RT118x/RT7xx devices)
- `libm7_cache.so`: specific to internal I/D-Cache of Cortex-M7 core (RT10xx/RT11xx devices)
