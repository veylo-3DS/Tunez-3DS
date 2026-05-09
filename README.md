# Tunez3DS

A feature-rich homebrew MP3 player for the Nintendo 3DS with a file browser, ID3 tag support, and embedded album art.

## v2.0 Features
- **UI Modernization ("Glass & Geometry")**: Card-based browser layout with depth effects and a refined "Now Playing" interface.
- **Playback Modes**: Normal, Repeat All, Repeat One, and Shuffle modes.
- **Enhanced Controls**: L/R shoulder buttons skip tracks; D-Pad L/R cycles playback modes.
- **Visual Identity**: Modernized icon, banner, and a custom startup jingle.

## Installation

1. For a seamless experience, install via [Universal Updater](https://github.com/Universal-Team/Universal-Updater) on your 3DS.
2. Alternatively, scan the QR code below using [FBI](https://github.com/Steveice10/FBI) to install the latest `.cia` file:

![Install via QR code](Tunez3DS_qr.png)

Or download the latest release directly from [GitHub](https://github.com/veylo-3DS/Tunez-3DS/releases/latest).

## Features

- **File browser** on the bottom screen — navigate folders and select tracks
- **Now Playing** screen on the top screen with title, artist, and a live progress bar with timestamp
- **ID3 tag support** — reads title and artist from MP3 metadata, falls back to filename if tags are absent
- **Album art** — displays embedded JPEG or PNG cover art from ID3 tags
- **Scrolling filenames** — long filenames scroll automatically when selected
- **Smart back navigation** — pressing B to leave a folder re-selects that folder in the parent directory

## Controls

| Button | Action |
|--------|--------|
| D-Pad Up / Down | Navigate file list |
| D-Pad L / R | Cycle playback modes |
| L / R Shoulder | Skip tracks |
| A | Open folder / Play track |
| X | Pause / Resume |
| Y | Stop playback |
| B | Go up one folder |
| START | Quit |

## Setup

Place your MP3 files on your SD card under `sdmc:/Music`. Subfolders are supported — you can navigate into them from the file browser.

## Supported Formats

- MP3 (`.mp3`)

## Building from Source

### Requirements

- [devkitPro](https://devkitpro.org/) with 3DS support (`devkitARM`)
- `libmpg123`, `libid3tag`, `libjpeg`, `libpng` (via devkitPro's pacman)
- `citro2d` / `citro3d`

### Build

```bash
make cia NO_SMDH=1
```
 