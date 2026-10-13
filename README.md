# Herri Kirolak Wii 🪓

Homebrew game for the Nintendo Wii inspired by Wii Sports, featuring Basque rural sports (*herri kirolak*). First event: **Aizkolaritza** (wood chopping).

Built with [devkitPro](https://devkitpro.org/) (devkitPPC + libogc). Runs on a real Wii through the Homebrew Channel, or in [Dolphin](https://dolphin-emu.org/).

> Personal side project, vibe-coded with Claude. UI text is in Basque (euskara).

## Build
1. Install devkitPro with the `wii-dev` group.
2. `make` (or `scripts/build.ps1` on Windows) → `build/boot.dol`

## Play on Wii
Copy `hbc/herri-kirolak/` + `boot.dol` to `apps/herri-kirolak/` on a FAT32 USB drive or SD card and launch it from the Homebrew Channel.

See [docs/SPEC.md](docs/SPEC.md) and [docs/ROADMAP.md](docs/ROADMAP.md).
