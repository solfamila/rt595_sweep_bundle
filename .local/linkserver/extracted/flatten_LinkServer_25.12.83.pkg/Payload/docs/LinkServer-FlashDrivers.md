# LinkServer Flash Drivers

*LinkServer* comes with a set of `Flash drivers` ([binaries/Flash](../binaries/Flash/)) which are used by LinkServer debug connections and flash operations.

The *LinkServer* debug server makes use of a RAM loadable `Flash driver` mechanism. Such a `Flash driver` contains the knowledge required to program the **internal Flash** on a particular MCU (or potentially, family of MCUs)  or the **external Flash** device(s) present on the board. This knowledge may be either hardwired into the driver, or some of it may be determined by the driver as it starts up (typically known as a `generic` Flash driver).

##  Per-Region Flash Drivers

By default, the devices JSON files ([devices](../devices/)) include a memory configuration with memory regions pre-configured with the appropriate `Per-Region` LinkServer Flash driver for the target flash device.

For most users, there is never any need to change the default configured Flash driver for the MCU/board being programmed.

If a particular Flash driver is needed for a memory region, it is recommended to add a new device entry to the existing JSON file or to create a new JSON file for that particular MCU/board configuration and set the particular `Per-region` Flash driver.

## Advanced Flash Drivers

Most of the LinkServer supported devices are already pre-configured with an appropriate flash driver for the target flash device. As a result, in many cases users need to pay little attention to the actual flash driver being used. However, for MCUs supporting complex flash strategies or external flash devices, the situation is more complex. This section discusses these situations but note, even in these cases, the flash driver may be automatically selected and so require no user attention.

### LPC18xx / LPC43xx Internal Flash Drivers

A number of LPC18/43 parts provide dual banks of internal Flash, with bank A starting at address 0x1A000000, and bank B starting at address 0x1B000000.

An appropriate per-bank flash driver is already configured for these parts. After programming the part, the selected flash driver will also configure it to boot from the corresponding bank.

**Note**: The selected flash driver depends on the memory region used at image build/link time.

### LPC SPIFI QSPI Flash Drivers

A number of parts provide support for external SPIFI Flash, sometimes in addition to internal Flash. Programming these Flash memories provides a number of challenges because the size of memory (if present) is unknown, and the actual memory device is also unknown. These issues are handled using Generic Drivers which can interrogate the memory device to find its size and programming requirements.

During a programming operation, the Flash driver will interrogate the SPIFI Flash device to identify its configuration. If the device is recognized, its size and name will be reported in the debug log - as below:
```
...
Inspected v.2 External Flash Device on SPI using SPIFI lib LPC18_43_SPIFI_GENERIC.cfx
Image 'LPC18/43 Generic SPIFI Mar 7 2017 13:14:25'
Opening flash driver LPC18_43_SPIFI_GENERIC.cfx
flash variant 'MX25L8035E' detected (1MB = 16*64K at 0x14000000)
...
```

**Note**: Although the Flash driver reports the size and location of the SPIFI device, the LinkSever's view of the world is determined by the memory configuration settings specified in devices JSON files. The users can update these settings to match the actual device in use.

**Note**: If a device is not supported by the LinkSever supplied Flash Drivers, sources to generate these drivers are supplied in the [Examples/Flashdrivers](../Examples/Flashdrivers/) subdirectory within the LinkServer installation directory. Users may thus add support for new SPIFI devices if needed.

### i.MX RT QSPI and Hyper Flash Drivers

I.MX.RT MCUs support external flash via a QSPI/Hyperbus interface, a range of LinkServer flash drivers supporting devices fitted to EVK development boards are included with LinkServer.

Note: some of the drivers are also supplied in source project form so they may be used as a base for development of drivers for other external flash parts. These driver projects can be found at [Examples/Flashdrivers/NXP/iMXRT](../Examples/Flashdrivers/NXP/iMXRT/).

**Important Note**: For an application to Boot and execute in place (XIP) from these flash devices (post reset), a correct header for the specific device **MUST be programmed into the flash (as part of the Project)**. SDK examples will build to include an appropriate header automatically however, the tools will not prevent users programming projects without headers into these devices. If this occurs the application will not boot and subsequent flash programming operations may fail.

Should this occur, the recommended recovery procedure is to change the boards boot strategy (via DIP switches) to prevent booting from QSPI or hyperflash. Power cycle the board and then perform a Mass Erase of the flash. Next, reprogram with an image that has appropriate header, restore the boot strategy and power cycle again.

**Tip**: In addition, these drivers are complemented by a range of self configuring drivers supporting all current iMX RT EVK boards, please see [Flash Drivers using SFDP](#Flash-Drivers-using-SFDP) for more information on the drivers and this methodology.

### Flash Drivers using SFDP (LPC, iMX RT and MCX)

As discussed above, the programming these Flash memories provides a number of challenges because the size of memory (if present) is unknown, and the actual memory device is also unknown.
LinkServer `Generic` flash drivers attempted to solve this problem by recognizing specific devices (via their JEDEC ID) and then setting their sizes and programming parameters accordingly. However, this mechanism only works if the device is recognized by the flash driver, and in consequence will fail if any device is not recognized.

This issue, combined with the sheer volume of devices available has forced a different approach to be taken. Fortunately, modern flash devices typically contain a data block describing their properties including device size, low level structure and programming details etc. These data blocks and their use are collectively known as Serial Flash Discovery Protocol or SFDP. The standard for these blocks are described by JEDEC JESD216 standard(s).

A range of `Generic` flash drivers built to self configure via SFDP data are available.

**Important Note**: for some parts, the JSON files/SDKs reference the device specific flash driver rather than the SFDP version. However you can modify your JSON/project to use the SFDP version if required. The Flash drivers cannot detect whether QSPI or Hyperflash is fitted on a board, therefore it is the responsibility of the user to ensure the correct driver is used.

**Note**: The iMX RT 1024 and 1064 MCUs incorporate a flash device within the MCU package itself however, the flash driver still uses the SFDP mechanism to detect the device.

#### QSPI SFDP issues and Limitations

Some (usually older) QSPI parts do not support the SFDP mechanism and therefore will not be programmable via this protocol. However since some of these QSPI devices are fitted to NXP (LPC) manufactured development boards, some basic assumptions are made by these drivers if SFDP data is not found. In such a case, the device and its size will be assumed to be 1MB and some standard programming mechanisms will be used. This scheme should ensure that NXP LPC development boards with QSPI can be used with this driver type.
**Note**: this information is correct at the time of writing and only applies to LPC Drivers - future development of these drivers may change their capabilities.

#### Flash programming log
When programming code or data into flash, a portion of the debug log will display the flash programming operations (as below):

|   | Log |
| - | --- |
| 1 | ```Inspected v.2 External Flash Device on SPI using SFDP JEDEC ID LPC18_43_SPIFI_SFDP.cfx``` |
|   | ```Sending VECTRESET to run flash driver```|
| 2 | ```Opening flash driver LPC18_43_SPIFI_SFDP.cfx``` |
|   | ```Sending VECTRESET to run flash driver ``` |
| 3 | ```flash variant 'JEDEC_SFDP_EF4014' detected (1MB = 16*64K at 0x14000000)``` |
|   | ```Closing flash driver LPC18_43_SPIFI_SFDP.cfx ``` |
|   | ```NXP: LPC43S37``` |
|   | ```Connected: was_reset=true. was_stopped=false``` |
|   | ```Awaiting telnet connection to port 3330 ...``` |
|   | ```GDB nonstop mode enabled``` |
| 4 | ```Opening flash driver LPC18_43_SPIFI_SFDP.cfx (already resident)``` |
|   | ```Sending VECTRESET to run flash driver``` |
| 5 | ```Writing 1046900 bytes to address 0x14000000 in Flash``` |
| 6 | ```Erased/Wrote page 0-15 with 1046900 bytes in 7548msec``` |
|   | ```Closing flash driver LPC18_43_SPIFI_SFDP.cfx``` |
|   | ```Flash Write Done``` |
| 7 | ```Flash Program Summary: 1046900 bytes in 7.55 seconds (135.45 KB/sec)``` |
|   | ```Stopped: Breakpoint #1``` |

**Note**: when accessing unknown flash devices, the driver will be called twice. First to identify the device and secondly to perform the required programming. In a situation where multiple devices are being programmed, the flash driver(s) may be (re)loaded for each use.

Where:
|   | Info |
| - | ---- |
| 1 | SFDP JEDEC ID is the method used to access the flash and LPC18_43_SPIFI_SFDP.cfx is the flash driver used |
| 2 | the driver named above is loaded and initialised (this step will setup clocks, pin muxing, and perform some investigation of the connected device) |
| 3 | the driver returns a string JEDEC_SFDP indicating that SFDP data was found and successfully read<br> - the devices JEDEC ID was read as EF4014, in this case corresponding to a Winbond 25Q80DVSIG (as fitted to the LPC-Link2 board used in Target mode)<br> - the devices size was read as 1MB divided up into 16 64KB Sectors/Blocks - these blocks are the erase size that will be used for programming and so any operation to program this flash must start on an address aligned to this 64KB size |
| 4 | the driver is opened a second time (without reloading since it remains from the previous call) |
| 5 | the project that referenced this driver requested that 1046900 bytes of data were written to the address starting 0x14000000, as set within the project's memory configuration |
| 6 | the write operation is performed via 16 page writes<br> **Note**: this flash driver (like many LinkServer drivers) uses a virtual page size that is much larger than the actual flash device page size to optimize driver operation |
| 7 | finally, a summary of the operation is printed showing the flash programming performance

**Note**: If the driver fails to find SFDP data, it will attempt to program the device with standard routines. If this occurs, the size will be assumed to be 1MB and the flash variant will be reported as ID rather than SFDP as shown below:
```
flash variant 'JEDEC_ID_EF4014' detected (1MB = 16*64K at 0x14000000)
```

On occasion, some devices that report the same JEDEC ID will actually be different, in this particular case the device is a very similar Winbond 25Q80BVSIG i.e. ..**BV** rather than ..**DV**.

#### QSPI Programming and Booting

When dealing with external flash, it is important to understand the difference between the flash programming operation performed by the flash driver and the subsequent use of the flash for executing code and/or providing data. Essentially the flash drivers responsibility ends with a successful program operation, after this point, correct operation of the MCU/SPI flash combination lies elsewhere.

Thus, once the MCU is reset (or power cycled), the responsibility for the devices configuration and operation lie entirely outside of tools and instead lie with one or all of the following:
- development board/MCU boot settings
  - these may be DIP switches or Jumpers providing inputs to the MCU boot flow, alternatively these could be OTP bits programmed within the MCU
- MCU's BootROMs ability to understand and setup the device
  - BootROMs on devices such as the LPC1800 and LPC4300 have inbuilt understanding of certain QSPI devices allowing them to be configured for boot. However, this boot process may fail with some QSPI flash despite the fact that it has been correctly programmed.
  - BootROMs on devices such as the LPC540xx and RT10xx rely on correct header (XIP) information being programmed (as part of the Application) into the QSPI flash itself. If this data is incorrect (or not present), the boot/reset will fail.
- Devices that incorporate both internal boot flash and external SPIFI/QSPI flash such as the LPC546xx typically place the responsibilities for QSPI configuration to the users application, where this might include
  - Setup of pinmuxing
  - QSPI/SPIFI clock setup
  - Flash interface initialization
  - QSPI initialization (this may be QSPI device specific)
    - including setup of appropriate waitstates for QSPI operation at the selected QSPI clock frequency

#### FlexSPI Flash reset

A number of IMX RT MCUs that support external flash via the FlexSPI interface implement a flash device reset sequence.

During FlexSPI boot the boot process requires the FlexSPI Flash device to be in a certain mode, for example, 1-bit SPI compatible mode. The Flash device will naturally be in this mode after a POR reset because the power-up sequence will reset it with the RT MCU device together.
However, the Flash device will not be in 1-bit SPI compatible mode if the flash device is configured to DPI mode or QPI mode or Octal mode when any non-POR resets happen. In such case, special processing is required by the boot process to restore the Flash device to 1-bit SPI compatible mode before continuing access to the Flash device. In general, this can be achieved by using a GPIO to assert a reset pin on the Flash device. The bootloader can perform the reset process and reset the Flash device to 1-bit SPI compatible mode based on fuses configuration, using the GPIO specified by the combination of FLEXSPI_RESET_PIN_PORT and FLEXSPI_RESET_PIN_GPIO.

When starting a flash-resident debug session this reset sequence may need to be performed by the flash driver as well. Flash drivers for IMX RT500 and RT600 MCUs implement this functionality.

**Note**: Custom boards may not be wired identical to EVK development boards in regards to the actual pin dedicated to flash device reset. In such cases the pre-connect script needs to be modified in order to pass to the flash drivers the relevant information about the GPIO pin used for flash reset.

## Kinetis Flash Drivers

Kinetis MCUs make use of a range of generic drivers.

Kinetis Flash drivers generally follow a simple naming convention i.e. FTFx_nK_xx where:
- FTFx is the Flash module name of the MCU, where x can take the value E, A or L
- nK represents the Flash sector size the Flash device supports, where n can take the value 1, 2, 4, 8
  - a sector size is the smallest amount of Flash that can be erased on that device
- xx represents an optional additional characters for special case drivers e.g. __Tiny for use on parts with a small quantity of RAM
  - a further optional _D suffix is used to show the driver is written to target Data Flash rather than the more common Program Flash

So for example a K64F MCU's Flash driver will be called FTFE_4K, because the K64F MCU uses the FTFE Flash module type and support a 4KB Flash sector size.

When a debug session is started that programs data into Flash memory, the debug log file will report the Flash driver used and parameters it has read from the MCU. Below we can see the driver identified a K64 part and the size of the internal Flash available. It also reports the programming speed achieved when programming this device. These logs can be useful when problems are encountered.

Note: when the Flash driver starts up, it will interrogate the MCU and report a number of data items. However, due to the nature of internal registers with the MCU, these may not exactly match the MCU being debugged.

```
Inspected v.2 On chip Kinetis Flash memory module FTFE_4K.cfx
Image 'Kinetis SemiGeneric Feb 17 2017 17:24:02'
Opening flash driver FTFE_4K.cfx
Sending VECTRESET to run flash driver
Flash variant 'K 64 FTFE Generic 4K' detected (1MB = 256*4K at 0x0)
Closing flash driver FTFE_4K.cfx
Connected: was_reset=true. was_stopped=true
Awaiting telnet connection to port 3330 ...
GDB nonstop mode enabled
Opening flash driver FTFE_4K.cfx (already resident)
Sending VECTRESET to run flash driver
Flash variant 'K 64 FTFE Generic 4K' detected (1MB = 256*4K at 0x0)
Writing 25856 bytes to address 0x00000000 in Flash
00001000 done 15% (4096 out of 25856)
00002000 done 31% (8192 out of 25856)
00003000 done 47% (12288 out of 25856)
00004000 done 63% (16384 out of 25856)
00005000 done 79% (20480 out of 25856)
00006000 done 95% (24576 out of 25856)
00007000 done 100% (28672 out of 25856)
Erased/Wrote sector 0-6 with 25856 bytes in 301msec
Closing flash driver FTFE_4K.cfx
Flash Write Done
Flash Program Summary: 25856 bytes in 0.30 seconds (83.89 KB/sec)
```

Flash drivers for a number of Kinetis MCUs are listed below:
```
K64F FTFE_4K (1MB)
K22F FTFA_2K (512KB)
KL43 FTFA_1K (256KB)
KL27 FTFA_1K (64KB)
K40 FTFL_2K (256KB)
```

## Command Line Flash Programmer

One of the main functionalities available within *LinkServer* are the `Flash operations` (i.e. erase, load, verify, etc.).

For details, please refer to the [Readme](../Readme.md) file, [Flash](../Readme.md/#Flash) paragraph.

### Examples of Flash operations from command line 

- Flash load: `./LinkServer flash MK64FN1M0xxx12:FRDM-K64F load frdmk64f_led_blinky.axf`
- Flash verify: `./LinkServer flash MK64FN1M0xxx12:FRDM-K64F verify frdmk64f_led_blinky.axf`
- Flash erase: `./LinkServer flash MK64FN1M0xxx12:FRDM-K64F erase`
- Flash blank check: `./LinkServer flash MK64FN1M0xxx12:FRDM-K64F blank -a 0x1000 -s 1024`

**Note**: *LinkServer* flash operations are using the Flash drivers pre-configured within the builtin devices JSON files. Please refer to the [Per-Region Flash Drivers](#per-region-flash-drivers) and [Advanced Flash Drivers](#advanced-flash-drivers) paragraphs in case a different configuration is needed.

**WARNING**: When LinkServer Flash drivers program data that they believe will form the start of an execute-in-place image they determine where the image's vector table is and automatically inserts a checksum of the initial few vectors, as required in many LPC parts. This may not be the value held in that location by the file from which the Flash was programmed. This means that if the content of the Flash were to be compared against the file a difference at that specific location may be found.

**WARNING**: Flash is programmed in sectors. The sizes and distributions of Flash sectors is determined by the Flash device used. Data is programmed in separate contiguous blocks. There may be many contiguous blocks of data specified in an ELF (.AXF) file but there is only one in a binary file. When a contiguous data block is programmed into Flash data preceding the block start in its Flash sector is preserved. Data following data in the block in the final sector, however is erased.

### Dealing with Errors during Flash operations

On some boards it is possible to run an image which is incompatible with the Flash driver. This incompatibility is likely to show in the form of programming errors signalled as the operation progresses. Often they are due to unmaskable exceptions (such as watchdog timers) being used by the previous image that interfere with a Flash driver's operation.

There are a number of ways to address this situation:
- Does your board support In System Processing (ISP) Reset? Using it will usually reset the hardware and stop in the Boot ROM, thus ensuring a stable environment for Flash drivers. If present it can usually be activated with one or more on-board switches. You may have to refer to the board's documentation.
- Erase the contents of Flash or program a (e.g. small) image that ensures no nonmaskable exceptions are involved. Naturally these solutions have the problem that they are as likely to fail (and for the same reason) as the programming operation. It is sometimes the case that an incompatible image will allow the Flash drivers to operate for a short period in which there is a chance that one of these 'solutions' can be used.
