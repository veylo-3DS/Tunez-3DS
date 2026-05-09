# Tunez3DS - Project Instructions

## Project Overview
Tunez3DS is a feature-rich homebrew MP3 player for the Nintendo 3DS. It is written in C and utilizes `citro2d`/`citro3d` for graphics and `ndsp` for high-quality audio playback.

### Recent Updates (v2.0)
- **UI Modernization ("Glass & Geometry")**: Card-based browser layout with depth effects and a refined "Now Playing" interface.
- **Playback Modes**: Normal, Repeat All, Repeat One, and Shuffle modes added.
- **Enhanced Controls**: L/R shoulder buttons skip tracks; D-Pad L/R cycles playback modes.
- **Visual Identity**: Modernized icon, banner, and a custom 3-second startup jingle.

## Architecture
- **`source/main.c`**: Entry point, input handling (HID keys, Touch, Shoulder triggers).
- **`source/audio.c`**: MP3 decoding, buffer management, playback queue, and shuffle/repeat logic.
- **`source/ui.c`**: Modern rendering logic using `C2D` for rounded cards, visualizer, and custom iconography.
- **`include/common.h`**: Shared structures, global configuration, and `MKCOL` macros.

## Release Workflow
When making a new release:
1. Update `README.md` with a summary of new features.
2. Ensure the QR code points to the newest release binary.
3. Tag the repository and create the GitHub release.

### Build Commands
- **Build 3DSX**: `make NO_SMDH=1`
- **Build CIA**: `make cia NO_SMDH=1`
- **Clean**: `make clean`

### Running
- **Hardware**: Install the `.cia` using FBI.
- **FTP Transfer**: You can transfer the `.cia` directly to the 3DS using the following command (ask user first!):
  `curl -T Tunez3DS.cia ftp://192.168.4.38:5000/Cias/Tunez3DS.cia --ftp-create-dirs`
  *(Note: ALWAYS ask the user before initiating this transfer, as they must manually open the FTP server on the 3DS first. Update the IP address above if it changes.)*
- **Emulator**: Use [Citra](https://citra-emu.org/) or [Azahar](https://github.com/Veylo/Azahar) to run the `.3dsx` file.
