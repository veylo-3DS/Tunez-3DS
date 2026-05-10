<div align="center">
  <img src="icon.png" alt="Tunez3DS Logo" width="128"/>
  <h1>Tunez3DS</h1>
  <p><strong>A modern, glass-aesthetic MP3 player for the Nintendo 3DS.</strong></p>
  
  <a href="https://github.com/veylo-3DS/Tunez-3DS/releases/latest">
    <img src="https://img.shields.io/github/release/veylo-3DS/Tunez-3DS.svg" alt="Latest Release"/>
  </a>
  <img src="https://img.shields.io/github/license/veylo-3DS/Tunez-3DS.svg" alt="License"/>
</div>

---

## ✨ Overview
Tunez3DS brings a refined, "Glass & Geometry" aesthetic to your 3DS music library. Built with `citro2d` and `citro3d` for hardware-accelerated graphics and `ndsp` for high-quality audio, it offers a smooth, responsive browsing and playback experience.

## 🚀 Key Features
- **Intuitive Touch Interface**: Navigate folders and play songs with a responsive, modern tap-to-act browser.
- **Hardware-Accelerated UI**: Fluid animations and depth effects powered by `citro2d`.
- **Customization**: Choose from 7 unique, system-inspired color themes.
- **Playback Control**: Adjust speed (0.25x - 4x) and modes (Shuffle, Repeat, etc.) on the fly.
- **Safety First**: Customizable lid-protection to prevent accidental track skips while in your pocket.
- **Easy Updates**: Check for the latest release directly within the settings menu.

## 📱 Quick Install
1. **[Download the latest CIA or 3DSX](https://github.com/veylo-3DS/Tunez-3DS/releases/latest)**.
2. Scan this QR code with [FBI](https://github.com/Steveice10/FBI):
   <br><br>
   <img src="Tunez3DS_qr.png" alt="Tunez3DS QR Code" width="200"/>

---

## 🎮 Controls

| Button | Action |
| :--- | :--- |
| **Touch** | Tap to Select/Open/Play |
| **D-Pad Up/Down** | Navigate file list / Adjust settings |
| **D-Pad L/R** | Cycle playback modes / Adjust speed |
| **L / R Shoulder** | Skip tracks / Switch settings pages |
| **A** | Play/Open / Apply settings |
| **X** | Pause/Resume / Reset speed |
| **Y** | Stop playback |
| **B / START** | Exit to browser / Quit application |
| **SELECT** | Open Settings |

---

## 🛠 Setup
Place your MP3 files on your SD card under `sdmc:/Music`. Subfolders are fully supported and can be navigated via the in-app file browser.

## 💻 Development
Requires [devkitPro](https://devkitpro.org/) with `devkitARM`.
- **Build 3DSX**: `make`
- **Build CIA**: `make cia NO_SMDH=1`

---
*Built for the 3DS community. Report issues via GitHub Issues.*
