# Tunez3DS

A feature-rich, open-source MP3 player for the Nintendo 3DS, designed for a modern and intuitive music listening experience.

![Tunez3DS Logo](icon.png)

## Notice
I'm not the best C dev in the world so if there is bugs, please make a github issue — and i will get it as fast as i can. Sometimes i will resopond fast, other times i wont, this is becuase this was mostly a side project for myself. But i decided to share it with other people if they wanted a proper Music Player for their 3DS!1!!

## Overview

Tunez3DS brings a refined, "Glass & Geometry" aesthetic to your 3DS music library. Built with `citro2d` and `citro3d` for hardware-accelerated graphics and `ndsp` for high-quality audio, it offers a smooth, responsive browsing and playback experience.

## Features

- **Intuitive Browser**: Overhauled touch controls with immediate selection and tap-to-open.
- **Software Updates**: Check for the latest version directly from the Settings menu.
- **ID3 Tag Support**: Automatically displays song titles, artists, and embedded album art.
- **Playback Versatility**: Supports Normal, Repeat All, Repeat One, and Shuffle modes.
- **Customization**: 7 unique system-inspired color themes.
- **Playback Speed**: Adjust audio speed from 0.25x to 4x in the **Settings menu** (SELECT).
- **Lid Protection**: Customizable safety setting to disable L/R skipping when the console is closed.
- **Smart Navigation**: Remembers folder state and supports intuitive B-button back navigation.

## Latest Release (v3.1.0)

- **Touch Interaction Overhaul**: The browser now feels much more responsive. Tapping an item immediately selects it, and releasing the tap opens the folder or plays the song.
- **Software Update Check**: Added a new settings page to check for newer versions on GitHub (requires Wi-Fi).
- **LED Feature Removal**: Removed the notification LED visualizer to focus on core performance and a cleaner settings menu.
- **Streamlined Settings**: Settings menu has been refined to 4 pages (Theme, Speed, Safety, Update).

## Installation

### Method 1: Universal Updater (Recommended)
Install directly via the [Universal Updater](https://github.com/Universal-Team/Universal-Updater) app on your 3DS.

### Method 2: Manual Installation
1. Download the latest `.cia` file from our [Releases page](https://github.com/veylo-3DS/Tunez-3DS/releases/latest).
2. Scan the QR code below using [FBI](https://github.com/Steveice10/FBI) to install:

![Install via QR code](Tunez3DS_qr.png)

## Controls

| Button | Action |
| :--- | :--- |
| **D-Pad Up/Down** | Navigate file list / Adjust settings |
| **D-Pad L/R** | Cycle playback modes / Adjust speed (Settings) |
| **L / R Shoulder** | Skip tracks / Switch settings pages |
| **A** | Open folder / Play track / Apply settings |
| **X** | Pause / Resume / Reset speed (Settings) |
| **Y** | Stop playback |
| **B** | Go up one folder / Back to browser |
| **START** | Quit application |
| **SELECT** | Open Settings |

## Setup

Place your MP3 files on your SD card under `sdmc:/Music`. Subfolders are fully supported and can be navigated via the in-app file browser.

## Building from Source

### Requirements
- [devkitPro](https://devkitpro.org/) with `devkitARM`
- `libmpg123`, `libid3tag`, `libjpeg`, `libpng`
- `citro2d` / `citro3d`

### Build Commands
- **Build 3DSX**: `make`
- **Build CIA**: `make cia NO_SMDH=1`
- **Clean**: `make clean`
