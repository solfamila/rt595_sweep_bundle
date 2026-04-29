# LinkServer integration with IDEs

[LinkServer](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/linkserver-for-microcontrollers:LINKERSERVER) is one of the components used by [MCUXpresso for Visual Studio Code extension](https://www.nxp.com/design/design-center/software/embedded-software/mcuxpresso-for-visual-studio-code:MCUXPRESSO-VSC) and [MCUXpresso IDE](https://www.nxp.com/design/design-center/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE) for debugging a target device.

Additional details regarding the IDEs can be found in the specific documentation, for example:
 - [MCUXpresso for VS Code extension](https://github.com/nxp-mcuxpresso/vscode-for-mcux)
 - [MCUXpresso IDE Documentation](https://www.nxp.com/design/design-center/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE#documentation)

This document describes the procedure to configure the *MCUXpresso for VS Code extension* and the *MCUXpresso IDE* to use a new/specific LinkServer version.
This is useful if a new LinkServer version is available, or when a specific version of LinkServer is needed.

First, download the new/specific LinkServer installer package and install it, then follow these additional steps:

## MCUXpresso for VS Code extension
VS Code automatically uses the latest LinkServer version detected in the default installation location.
The following steps are necessary when installing an older LinkServer version or if LinkServer is installed in a non-default path.

1. Open the VS Code.
2. Open the **Settings** editor from the *Command Palette* (`Ctrl+Shift+P`) with *Preferences: Open Settings* and search **mcuxpresso.linkserver.path**.
3. Set the new/custom path to LinkServer in **Linkserver: Path** field.
4. *Refresh the Debug probes* in the *DEBUG PROBES* panel.


## MCUXpresso IDE

### Associating at LinkServer install time
The simplest way to associate with MCUXpresso IDE is at the time a new LinkServer version is installed. The installer allows selecting from compatible MCUXpresso IDE installations detected on the system, which will be automatically configured to use the LinkServer version being installed.

The following sections are useful if additional changes are needed at a later time after LinkServer installation, or for older LinkServer versions.

### Configuring default LinkServer version globally

**Note**: The following command changes the default LinkServer version used in the specified MCUXpresso IDE and applies for any workspace; therefore, this approach is recommended over the "current workspace only" method described later.

1. Close the MCUXpresso IDE if it is open.
2. Navigate to the new LinkServer installation path and execute the following command:\
`./LinkServer maint ide associate <path_to_MCUXpressoIDE_installation_folder>`

where:
 - *<path_to_MCUXpressoIDE_installation_folder>* is the folder where the MCUXpresso IDE is installed.

Example:\
`./LinkServer maint ide associate C:\NXP\MCUXpressoIDE_24.9.25`

Or use `./LinkServer maint ide select` to show compatible MCUXpresso IDE installations and interactively select the desired one.
Similarly, `./LinkServer gui maint` opens a GUI interface for selecting MCUXpresso IDE installations.

This changes the **Default path** to LinkServer. To verify:
 - Open the MCUXpresso IDE.
 - Go to
   - *MCUXpresso IDE -> Window -> Preferences...* for Windows/Linux
   - *MCUXpresso IDE -> Settings...* for macOS
- Expand MCUXpresso IDE -> Debug Options -> **LinkServer Options** category and check that **Default Path** inside the LinkServer path configuration section is set to the selected LinkServer location.

### Configuring default LinkServer version globally (alternative)

This alternative method does not use LinkServer `maint ide` commands and is useful for older LinkServer versions.

1. Close the MCUXpresso IDE if it is open.
2. Execute the following command:\
`<path_to_MCUXpressoIDE_installation_folder>\ide\mcuxpressoide -application com.nxp.mcuxpresso.headless.application -nosplash -run set.config.preference com.nxp.mcuxpresso.core.debug.support.linkserver:linkserver.path.default_path=<path_to_LinkServer_installation_folder>`

where:
 - *<path_to_MCUXpressoIDE_installation_folder>* is the folder where the MCUXpresso IDE is installed.
 - *<path_to_LinkServer_installation_folder>* is the folder where the new/custom LinkServer is installed.

Example:\
`C:\NXP\MCUXpressoIDE_24.9.25\ide\mcuxpressoide -application com.nxp.mcuxpresso.headless.application -nosplash -run set.config.preference com.nxp.mcuxpresso.core.debug.support.linkserver:linkserver.path.default_path=c:\NXP\LinkServer_24.10.15`

### Configuring LinkServer for the current workspace only

The MCUXpresso IDE has a workspace-level user preference which can be used to override the default LinkServer version by pointing to a custom installation path.

**Note**: This preference needs to be configured for any new workspace.

1. Open the MCUXpresso IDE.
2. Go to
   - *MCUXpresso IDE -> Window -> Preferences...* for Windows/Linux
   - *MCUXpresso IDE -> Settings...* for macOS
3. Expand *MCUXpresso IDE -> Debug Options -> **LinkServer Options*** category.
4. Enable **Custom Path** inside the **LinkServer path configuration** section.
5. Browse to the new/specific LinkServer installation folder.
