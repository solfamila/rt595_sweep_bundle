# LinkServer ReleaseNotes

## 25.12.83
### New features
- Upgraded MCU-Link version to 3.167.
- New Linux ARM64 host architecture support.
- Upgraded LPCScrypt version to 2.1.4_98.
- Added support for MCXN557S device.
- Added support for MCXA173, MCXA174 devices and FRDM-MCXA174 board.
- Added preliminary support for Cortex-M55 and Cortex-M85 cores.
- Added standalone entries (no board) for **Kinetis** devices from the following families:
    - **K32L2A41A**: K32L2A31xxxxA
    - **K32L2B11A**: K32L2B11xxxxA
    - **K32L2B21A**: K32L2B21xxxxA
    - **KW4x**: KW45B41Z52xxxA, KW45B41Z53xxxA, KW45B41Z82xxxA, KW45Z41053xxxA, KW45Z41082xxxA, KW45Z41083xxxA
    - **K0x**: MK02FN64xxx10, MK02FN128xxx10
    - **K2x**: MK22FN128xxx10, MK22FN128xxx12, MK22FN256xxx12
    - **KE1x**: MKE12Z128xxx7, MKE12Z256xxx7, MKE13Z128xxx7, MKE13Z256xxx7, MKE14Z128xxx7, MKE14Z256xxx7, MKE14Z32xxx4, MKE14Z64xxx4, MKE15Z32xxx4, MKE15Z64xxx4
    - **KM3x**: MKM14Z64Axxx5, MKM14Z128Axxx5, MKM33Z64Axxx5, MKM33Z128Axxx5
- Added RT1180 flash drivers for Micron Octal flash.
- [CLI, LinkFlash] Added support for selecting which flash memories to erase (if there are multiple).
- [CLI] Added `gdbserver --no-rtos` option for disabling RTOS thread awareness.

### Improvements
- New MCU-LINK_installer build for macOS with native Apple silicon support.
- New LPCScrypt build for macOS with native Apple silicon support.
- General performance improvements for LinkServer.
- Probes configured with debug function disabled are no longer listed.
    - Added *Capabilities* column to `probes` output.
    - Added `probes --capab` option for filtering probes with specific capabilities.
- Replaced `flash regions` command with `device info`. The new command no longer requires a connected probe.
- Replaced `config` command with `device export`. Replaced the `--output` option with a mandatory positional argument.

### Bug fixes
- Fixed hang when WINUSB-based probes fail to communicate.
- Fixed intermittent hang when using HID-based probes with latest macOS version.
- Fixed port allocation issue when IPv6 is disabled.
- Fixed handling of Ctrl-C and `--pause` in runner.
- Fixed potential heap corruption during flash programming.
- Fixed placing XMCD data inside the boot header section of RT1170/RT1160 targets.
- Fixed SYSRESET being ignored from non-secure state.


## 25.9.134
### New features
- Upgraded MCU-Link version to 3.165.
- Added initial support for MCXA185, MCXA186, MCXA265, MCXA266 devices and FRDM-MCXA266 board.
- Added initial support for MCXA343, MCXA344, devices and FRDM-MCXA344 board.
- Added initial support for MCXA365, MCXA366 devices and FRDM-MCXA366 board.
- Added initial support for MIMXRT1186 device and FRDM-IMXRT1186 board.
- Added support for MCXN247, MCXN526, MCXN527, MCXN536, MCXN537, MCXN556S devices.

### Improvements
- Improved MCXE31x reset script for cases like post-download reset after flash erase.
- Switched to using rapid blhost (rblhost) utility.

### Bug fixes
- Updated MCXE31x connect script to use part specific SRAM size for SRAM ECC initialization.
- Resolved an issue with TCM1 memory access during debug mode on MCXE31x devices.
- Fixed `ConnectionResetError` after low-level `wireswdconnect` command.
- Fixed desktop shortcut creation on Windows for non-admin users with space in name.
- Fixed probe booting with the latest macOS version.
- Improved error message related to specifying devices on command-line.


## 25.7.33
### New features
- Added initial support for MCXL255, MCXL254, MCXL253 devices and FRDM-MCXL255 board.

### Improvements

### Bug fixes
- Updated RT1060 QSPI flash loader to work with updated FCB settings in SDK 25.09 for RT1060-EVKB boards.


## 25.6.131
### New features
- Upgraded MCU-Link version to 3.160.
- Upgraded blhost version 3.0.0 (spsdk)
- Added support for MCXW235, MCXW236 devices.
- Added initial support for MCXA345, MCXA346 devices and FRDM-MCXA346 board.
- Added initial support for MCXA175, MCXA176, MCXA255, MCXA256, MCXA355, MCXA356 devices.

### Improvements
- Updated RT1160/RT1170 support for rev C0 devices.
- Added SWO trace support for MCXE31x devices.
- Updated MCXE31x support to not show the secondary Cortex-M7 core (not present on these devices).
- Added support to allow custom pre-attach initializations (needed for MCXE31x devices).
- Added --exit-timeout option for runner to limit the time it waits for application termination.
- Reduced installation size and remove reboot prompts on Windows.
- Upgraded LinkServer launcher to use Python 3.13.3.
- Upgraded libexpat version 2.7.1.
- Discontinued support for legacy RedProbe and RedProbe+ probes.
- Improved device verification based on CMSIS-DAP target information (if available).
- Updated visual style for product icons.

### Bug fixes
- Fixed a problem that might affect RT700 connections when multiple probes are connected to the PC.
- Fixed a problem preventing LinkFlash to start when auto-saved configuration references missing files.
- Fixed a problem related to incorrect command sequence being displayed for LinkFlash Save operations.
- Fixed a problem related to terminal settings handling in runner when stdin is not connected to a terminal.


## 25.3.31
### New features
- Upgraded MCU-Link version to 3.156.
- Added initial support for FRDM-MCXW23 and MCXW23-EVK.
- Added initial support for MCXE247, MCXE246, MCXE245 devices and FRDM-MCXE247 board.
- Added initial support for MCXE31B, MCXE317, MCXE316, MCXE315 devices and FRDM-MCXE31B board.
- Added possibility to associate LinkServer with existing MCUXpresso IDE installations at install time.

### Improvements
- Added new maintenance command for GUI management of integration with MCUXpresso IDEs.
- Added standalone entries (no board) for **MCX** devices from the following families:
    - **MCXC14x/24x/44x**: MCXC141, MCXC142, MCXC143, MCXC144, MCXC242, MCXC243, MCXC244, MCXC443, MCXC444
    - **MCXW7XX**: MCXW716AxxxA
- Added standalone entries (no board) for **LPC** devices from the following families:
    - **LPC546XX**: LPC54605J256, LPC54605J512, LPC54606J256, LPC54606J512, LPC54607J256, LPC54607J512, LPC54616J256, LPC54616J512
    - **LPC540XX**: LPC54S005, LPC54S016, LPC54005, LPC54016, LPC54018J2M, LPC54018J4M
    - **LPC550x/S0x**: LPC55S04, LPC5502, LPC5504, LPC5506
    - **LPC551x/S1x**: LPC55S14, LPC5512, LPC5514, LPC5516
    - **LPC552x/S2x**: LPC55S26, LPC5526, LPC5528
    - **LPC553x/S3x**: LPC5534, LPC5536
    - **LPC55S6x**: LPC55S66
    - **LPC86x**: LPC864
- Added standalone entries (no board) for **i.MXRT** devices from the following families:
    - **MIMXRT500**: MIMXRT533S, MIMXRT555S
    - **MIMXRT600**: MIMXRT633S
    - **MIMXRT700**: MIMXRT735S
    - **MIMXRT1050**: MIMXRT1051B
    - **MIMXRT1060**: MIMXRT1061A, MIMXRT1061B, MIMXRT1064B, MIMXRT1165
    - **MIMXRT1170**: MIMXRT1171, MIMXRT1172, MIMXRT1173, MIMXRT1175, MIMXRT117T, MIMXRT117H, MIMXRT117F, MIMXRT117C
    - **MIMXRT1180**: MIMXRT1181, MIMXRT1182, MIMXRT1187
- Added standalone entries (no board) for **i.MX** devices from the following families:
    - **MIMX93xx**: MIMX9301, MIMX9302, MIMX9311, MIMX9312, MIMX9321, MIMX9331, MIMX9332, MIMX9351
    - **MIMX959x**: MIMX9596
- Added FlexSPI2 flash loaders for RT1180
- Improve USB probe listing performance on Windows
- LinkServer flash and run commands detect file type automatically for unknown file extensions

### Bug fixes
- Fixed LinkServer runner sending incorrect line ending when user presses ENTER interatively in the console on Windows.
- Fixed LinkServer runner `--pause` option requiring multiple key presses.
- Fixed programming MCXA2xx/MCXA3xx parts which reserve the final part of the flash range for Secure Installer functionality.
- Fixed debug connection to KL28Z and LPC51U68 devices.
- Fixed LinkServer inadvertently reusing the same (telnet) port in multiple concurrent instances.
- Fixed handling of semihosting operations where the parameter block may start on unaligned target addresses.


## 24.12.21
### New features
- Upgraded MCU-Link version to 3.153.
- Added initial support for KW47, MCXW72 devices and KW47-EVK, KW47-LOC, MCX-W72-EVK, FRDM-MCXW72, MCX-W72-LOC boards.
- Added Cortex-M debug support for i.MX95 and i.MX93.

### Improvements
- Documented [LinkServer GDB server](docs/LinkServer-GDBServer.md) monitor commands.
- Documented [LinkServer integration with IDEs](docs/LinkServer-IntegrationWithIDEs.md).
- Added inline format description inside [probetable.csv](binaries/Scripts/probetable.csv) file.
- Added option for runner to wait for a specific marker before sending (non-interactive) input to application.
- Added interactive input capability to the runner.
- Added LinkServer auto device selection and verification based on CMSIS-DAP target information (if available)
- Added LinkFlash launcher and updated the installer to create LinkFlash shortcuts.
- Added LinkFlash auto device selection and verification based on CMSIS-DAP target information (if available)
- Added new maintenance commands category and subcommands for managing integration with MCUXpresso IDEs.
- Updated override option to allow creating intermediary paths.
- Added support for using shorter/simpler names as alternatives to ids for target device selection.
- Replaced usage of .vbs scripts with PowerShell scripts due to VBScript deprecation in Windows.
- Updated several LinkServer scripts (.scp) to temporarily reduce the wire speed while performing reset.
- Improved locating preconnect, connect and reset scripts relative to current directory.
- Added standalone entries (no board) for MIMXRT1040 devices (MIMXRT1041, MIMXRT1042, MIMXRT1043, MIMXRT1046).

### Bug fixes
- Fixed missing RAM regions for MIMXRT1160 and MIMXRT1170.
- Updated the connect scripts and flash drivers for MIMXRT1180 to avoid flash driver initialization issues in some circumstances.
- Updated MIMXRT1180 memory map (flash regions declared as Cortex-M33 regions) to allow debug of standalone Cortex-M7 examples in RAM.
- Correctly identify reset context passed to reset scripts when multiple flash ranges are present (this was affecting secure flash images on RT700).
- Fixed a problem seen in certain circumstances for list probes command (caused be some USB devices reported on Windows).
- Fixed some problems related to obtaining dapinfo from probes.


## 24.9.75
### New features
- Upgraded MCU-Link version to 3.148.
- Added initial support for MIMXRT700.
- Add LinkServer flash programming utility for executing flash operations using the graphical user interface.
  - Refer to [LinkFlash](docs/LinkFlash.md) for additional details regarding the GUI flash support.
- Add LinkServer application runner for use in CD/CI testing environments.
- LinkServer flash load and flash verify commands now accept multiple files as arguments.

### Bug fixes
- Fixed a MCXN9xx FlexSPI flash driver problem (reproducible on some FRDM-MCXN947 boards).
- Fixed RT1020, RT1160/RT1170/RT1180 flash drivers in SDP mode.

### Improvements
- LinkServer binaries are now compiled as 64-bit applications on Windows.
- Install x86 and x64 versions of Microsoft Visual C++ Redistributable on Windows.
- Added MCXN9xx FlexSPI flash driver to examples.
- Added disable GDET sequence to MCXN9xx and MCXN5xx preconnect scripts.
- Added standalone entries (no board) for MCXN9xx, MCXN5xx, MCXN2xx, MCXA1xx devices.


## 1.6.133
### New features
- Added support for FRDM-MCXW71 board.


## 1.6.121
### New features
- Upgraded MCU-Link version to 3.146.
- Added support for MCXC041, MCXC242, MCXC444 devices and FRDM-MCXC041, FRDM-MCXC242, FRDM-MCXC444 boards.
- Added support for MCXW71x devices and FRDM-MCXW7X board.
- Added support for FRDM-RW612 board.
- Added initial support for FreeRTOS v11 and newer FreeRTOSDebugConfig.

### Improvements
- Increased the maximum number of supported probes in redlinkserv from 8 to 16.
- Updated RW612 debug support for A2 silicon.
- Updated LinkServer installer on Linux to keep multiple instances.

### Bug fixes
- Fixed the message reported on list probes command in case of server error 0xAB (Exceeded maximum number of connected probes).
- Fixed problems related to evaluation of complex expressions in LinkServer scripts (.scp).
- Fixed a problem related to soft reset for MIMXRT1170 and MIMXRT1160 when debugging Cortex-M4 applications in RAM.
- Fixed a problem related to restart after flash load.


## 1.5.30
### New features
- Upgraded MCU-Link version to 3.140.
- Added support for FRDM-MCXA156.
- Added flash support for S19 and Intel Hex files.

### Improvements
- Fixed execution via PATH on Linux and macOS.
- Updated probes listing on Windows to show the MCU-Link probes opened by another LinkServer process.
- Updated memory configuration for several devices (based on Cortex-M33) to include the secure RAM regions.
- Automatic firmware update support for MCU-Link Mini probes.
- Added an option for rltool to set the redlinkserv port.


## 1.4.85
### New features
- Upgraded MCU-Link version to 3.133.
- Added support for FRDM-MCXN947.
- Added support for MCXN2xx devices and FRDM-MCXN236 board.
- Added support for automatic firmware update for MCU-Link probes.
- Added support to show CMSIS-DAP probe information.
- Added probe commands to run a low-level script, issue a timed reset.
- Added support to dump the content of a flash region to a file.
- Added ProbeReset and ProbeDAPInfo commands to redlinkserv.
- Integration support for MCUXpresso IDE 11.9.x and later.
  The following artifacts have been moved from IDE within LinkServer installation:
  - CoreSight components configuration files: [binaries/coresight](./binaries/coresight/).
  - LPC-Link2 power measurement circuits configuration files: [binaries/power](./binaries/power/).
  - Default linker templates (.ldt) files: [Wizards/linker](./Wizards/linker/).
  - Flash driver examples: [Examples/Flashdrivers](./Examples/Flashdrivers/).

### Improvements
- Updated the connect scripts for MIMXRT1180.
- Flush cache before reading memory for semihosting operations.

### Bug fixes
- Fixed additional problems related to flash blank command in GDB server and flash drivers (LPC55xx).
- Fixed a semihosting file operation problem (wrong file position after SYS_FLEN system call).


## 1.3.15
### New features
- Upgraded MCU-Link version to 3.128.
- Added support for KE1xZ512 devices and X-FRDM-KE17Z512.
- Added support for MIMXRT1180-EVK board.
- Added support for MCXA153 device and FRDM-MCXA153 board.
- Added support for API versioning (MAJOR.MINOR), intended to be used for compatibility checks.

### Improvements
- Log flash output to stdout.
- Document the Flash drivers support.
- Added --keep-alive option for gdbserver.

### Bug fixes
- Correction of flash driver for eeprom flash locations in CTN73x/PN73xx/PN74xx.
- Correction of RW61x connect script (ignore secure bit in entry address range check).
- Fixed issues with `--script=<filename>` argument for redlinkserv.
- Fixed issues with memory regions overlap between cores.
- Fixed some problems related to flash blank command.


## 1.2.45
### New features
- Upgraded LPCScrypt version to 2.1.3_83.
- Upgraded MCU-Link version to 3.122.
- Upgraded LinkServer LPC-Link2 firmware v5.460 which offers support for powering RT1xxx EVK boards through the USB debug connection.
- New LinkServer build for Mac with native Apple silicon support.
- Added support for MCXN9xx.
- Added support for RW61x.
- Added support for some missing (legacy) boards.
### Improvements
- Removed the kits from the boards list.
- Document the low level functions and scripting support.
- Erase each flash region only once.
- Enable register caching on GDB $p packet.
- Kinetis resurrect is missing for a few devices.
### Bug fixes
- Flash tool not writing flash config. section from ELF.
- Overflow problems in DAP_Transfer commands.
- Problems related to semihosting operations.
- Cannot boot LPC-Link1 when a LPC-Link2 probe configured for DFU booting is also present.

## 1.1.16
### Improvements
- Install VS2015 C++ Redistributables on Windows.
### Bug fixes
- Secure flash regions are missing.

## 1.0.9 (First release)
