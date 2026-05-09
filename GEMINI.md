# Tunez3DS - Project Instructions

## Project Overview
Tunez3DS is a feature-rich homebrew MP3 player for the Nintendo 3DS. It is written in C and utilizes the `citro2d`/`citro3d` libraries for graphics and `ndsp` for high-quality audio playback via `libmpg123`.

### Architecture
The project follows a modular procedural architecture:
- **`source/main.c`**: Entry point, main loop, and unified input handling (HID keys and Touch screen).
- **`source/audio.c`**: Manages MP3 decoding, buffer management, playback state, and real-time visualizer calculations. Implements a persistent playback queue for robust auto-advance.
- **`source/browser.c`**: Handles SD card file system scanning, alphabetical sorting (folders first), and directory navigation logic.
- **`source/ui.c`**: Contains all rendering logic for the Top, Bottom, and Settings screens. Manages the theme system, ID3 tag extraction (Title, Artist, Album), and embedded cover art loading.
- **`include/common.h`**: The central header defining shared structures (`Entry`, `Theme`), global state, and configuration constants.

### Key Technologies
- **Toolchain**: devkitPro / devkitARM
- **Audio**: `libmpg123`, `ndsp`
- **Graphics**: `citro2d`, `citro3d`
- **Metadata**: `libid3tag`
- **Images**: `libjpeg`, `libpng`

## Building and Running

### Build Commands
The project uses a standard devkitPro Makefile.
- **Build 3DSX**: `make NO_SMDH=1`
- **Build CIA**: `make cia NO_SMDH=1`
- **Clean**: `make clean`

### Running
- **Hardware**: Copy the `.3dsx` to `/3ds/` on your SD card or install the `.cia` using FBI.
- **FTP Transfer**: You can transfer the `.cia` directly to the 3DS using the following command:
  `curl -T Tunez3DS.cia ftp://192.168.4.38:5000/Cias/Tunez3DS.cia --ftp-create-dirs`
  *(Note: ALWAYS ask the user before initiating this transfer, as they must manually open the FTP server on the 3DS first. Update the IP address above if it changes.)*
- **Emulator**: Use [Citra](https://citra-emu.org/) or [Azahar](https://github.com/Veylo/Azahar) to run the `.3dsx` file.

## Development Conventions

### Code Style
- **Procedural C**: Keep logic focused and separated into the appropriate modules (`audio`, `browser`, `ui`).
- **Global State**: Global variables are declared in `include/common.h` and defined in their respective `.c` files. Use them judiciously.
- **Memory Management**: Use `linearAlloc`/`linearFree` for audio buffers and other hardware-accessible memory. Always cleanup resources (textures, file handles) on exit.

### UI & Layout
- **Resolution**: Top screen is 400x240, Bottom screen is 320x240.
- **Themes**: New themes should be added to the `themes` array in `source/ui.c`. Colors are manually packed using the `MKCOL(r,g,b,a)` macro.
- **Icons**: Use the custom drawing primitives in `source/ui.c` (e.g., `drawFolderIcon`, `drawNoteIcon`) to maintain a consistent look.

### Functional Limits
- **File Limit**: `MAX_FILES` is set to 1024. Increasing this significantly may impact memory and performance during folder scans.
- **Audio Buffers**: Uses two `4096` sample buffers (stereo 16-bit PCM) for double-buffering.

### Contribution Guidelines
- Ensure any UI changes are tested for both D-Pad and Touch responsiveness.
- When adding new metadata support, update both the ID3 loader in `ui.c` and the display logic in `drawTopScreen`.
- If modifying playback logic, verify that the `autoAdvance` queue remains consistent when the user changes directories in the browser.
