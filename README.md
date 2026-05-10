# Tunez3DS

A feature-rich, open-source MP3 player for the Nintendo 3DS, designed for a modern and intuitive music listening experience.

![Tunez3DS Logo](icon.png)

## Notice
I'm not the best C dev in the world so if there is bugs, please make a github issue — and i will get it as fast as i can. Sometimes i will resopond fast, other times i wont, this is becuase this was mostly a side project for myself. But i decided to share it with other people if they wanted a proper Music Player for their 3DS!1!!

## Overview

Tunez3DS brings a refined, "Glass & Geometry" aesthetic to your 3DS music library. Built with `citro2d` and `citro3d` for hardware-accelerated graphics and `ndsp` for high-quality audio, it offers a smooth, responsive browsing and playback experience.

## Features

- **Intuitive Browser**: Card-based interface with depth effects and smooth scrolling.
- **ID3 Tag Support**: Automatically displays song titles, artists, and embedded album art.
- **Playback Versatility**: Supports Normal, Repeat All, Repeat One, and Shuffle modes.
- **Customization**: 7 unique system-inspired color themes.
- **Playback Speed**: Adjust audio speed from 0.25x to 4x in the **Settings menu** (SELECT). Use **X** to increase and **Y** to decrease speed.
- **Smart Navigation**: Remembers folder state and supports intuitive B-button back navigation.

## Latest Release (v2.6.6)

- **UI Improvements**: Relocated playback speed controls to the Settings menu.
- **Improved Controls**: Use **X** and **Y** to adjust speed in Settings.
- **Navigation**: Restored original D-Pad navigation functionality.

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
| **D-Pad Up/Down** | Navigate file list |
| **D-Pad L/R** | Cycle playback modes |
| **L / R Shoulder** | Skip tracks |
| **A** | Open folder / Play track |
| **X** | Pause / Resume |
| **Y** | Stop playback |
| **B** | Go up one folder |
| **START** | Quit application |

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
