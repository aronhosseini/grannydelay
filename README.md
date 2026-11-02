# Granny Delay VST3 Plugin

A modern, granular delay audio effect plugin with a stunning 3D animated UI featuring an interactive isolator visualizer. Available as VST3, AU, and Standalone for Windows and macOS.

**Developed by [Aron H](https://example.com)**

## Features

- **Granny Delay Engine**: Slice audio into grains with variable size, density, pitch, and position
- **3D Isolator Visualizer**: Animated 3D cube grid that responds to your parameters in real-time
- **Modern UI**: Dark theme with animated background, neon accent colors, and glowing controls
- **11 Parameters**: Delay Time, Feedback, Mix, Grain Size, Grain Density, Pitch, Spread, Isolator, Space, Mod Rate, Mod Depth
- **Formats**: VST3, Audio Unit (AU), Standalone
- **Platforms**: Windows (VST3), macOS (VST3, AU, Standalone)

## Building from Source

### Prerequisites

- [JUCE Framework](https://juce.com/) 7.0.12 (cloned into `./JUCE`)
- CMake 3.22+
- **Windows**: Visual Studio 2022 with the C++ workload (MinGW is not supported)
- **macOS**: Xcode 12+, macOS 10.13+

### Setup

1. Clone this repository:
   ```bash
   git clone https://github.com/aronh/GrannyDelay.git
   cd GrannyDelay
   ```

2. Clone JUCE next to the sources (or pass `-DJUCE_PATH=/path/to/JUCE`):
   ```bash
   git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git JUCE
   ```

3. Configure and build:

   **Windows (Visual Studio):**
   ```powershell
   cmake -S . -B build -A x64
   cmake --build build --config Release
   ```

   **macOS:**
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```

   The plugin ends up in `build/GrannyDelay_artefacts/Release/VST3/GrannyDelay.vst3`
   (on macOS with the default Makefile generator: `build/GrannyDelay_artefacts/VST3/`).

### Build Scripts

- `scripts/build-windows.ps1` - Windows build script
- `scripts/build-mac.sh` - macOS build script

## Installers

Create a GitHub **Release** (tag e.g. `v1.0.0`); the CI attaches these files automatically:

- **Windows**: `GrannyDelay-Setup.exe` (installs to `C:\Program Files\Common Files\VST3`) or `GrannyDelay-Windows-VST3.zip`.
  Unsigned installers show a SmartScreen warning: click *More info > Run anyway*.
- **macOS**: `GrannyDelay-macOS.dmg` / `GrannyDelay-macOS.pkg`, `GrannyDelay-macOS.zip`, `install.sh`, `granny-delay.rb`

### macOS: install without the Gatekeeper warning

Plugins are ad-hoc signed in CI, and these two routes never set the quarantine flag, so macOS does not block them:

1. **One-line installer** (paste into Terminal; user-level install, no password):
   ```bash
   curl -fsSL https://github.com/YOUR_USER/YOUR_REPO/releases/latest/download/install.sh | bash
   ```
   Add `-s -- --system` after `bash` for an all-users install, or `-s -- --uninstall` to remove it.
   (The `install.sh` attached to each release already has your repository filled in.)

2. **Homebrew**: create a repository called `homebrew-granny`, put `granny-delay.rb` from the release into
   `Casks/`, then:
   ```bash
   brew tap YOUR_USER/granny
   brew install --cask granny-delay
   ```
   The cask also strips the quarantine flag after install.

If you download the `.dmg` / `.pkg` in a browser, macOS quarantines it. Open *System Settings > Privacy & Security >
Open Anyway*, or run `xattr -dr com.apple.quarantine ~/Downloads/GrannyDelay-macOS.pkg`.

**Fully warning-free double-click installs** need an Apple Developer ID ($99/year). Add these repository secrets and CI
will sign, notarize and staple the package automatically: `APPLE_CERT_P12` (base64 of a .p12 containing the
*Developer ID Application* and *Developer ID Installer* certificates), `APPLE_CERT_PASSWORD`, `APPLE_APP_IDENTITY`,
`APPLE_INSTALLER_IDENTITY`, `APPLE_ID`, `APPLE_TEAM_ID`, `APPLE_APP_PASSWORD` (app-specific password).

## Parameter Reference

| Parameter | Range | Description |
|-----------|-------|-------------|
| Delay Time | 0-2000 ms | Delay line time |
| Feedback | 0-95% | Amount of output fed back into delay |
| Mix | 0-100% | Dry/wet balance |
| Grain Size | 5-500 ms | Duration of each grain |
| Grain Density | 1-100 /s | Grains spawned per second |
| Pitch | -12 to +12 st | Pitch shift in semitones |
| Spread | 0-100% | Stereo spread of grains |
| Isolator | 0-100% | Resonant low-pass sweep on the wet signal (20 kHz down to ~400 Hz) |
| Space | 0-100% | Reverb amount on the wet signal |
| Mod Rate | 0.01-20 Hz | Modulation LFO rate |
| Mod Depth | 0-100% | Modulation depth |

## License

MIT License. See [LICENSE](LICENSE) for details.

## Author

[Aron H](https://example.com)

## Screenshots

_Add screenshots of the plugin UI here_

---

**Granny Delay** - Where grains meet space. 🌌


## Features (v1.0)

**Time** - delay time in ms, or tempo sync (SYNC button + division knob, follows the host BPM), feedback, mix.

**Grains** - size, density, pitch, spread, pan, plus **motion** (per-grain pan travel, scatter and detune drift),
**saturation**, **freeze** (holds the buffer so grains keep playing it), **stretch** (read-head speed vs real time),
**position** (start offset into the stored audio) and **distance** (quieter, darker, more reverb).

**Filter** - 10 types: LP 12, LP 24, HP 12, HP 24, Band, Notch, Peak, Low Shelf, High Shelf, All-pass (cutoff, resonance, gain).

**Chord resonator** - 4 tuned resonators: chord type (10), root note, strum **time**, **decay**, **gain**, **mix**.

**Reverb** - 5 types (Room, Plate, Hall, Cathedral, Space) with decay, mix, high damping, low damping and pre-decay.

**Modulation** - 2 LFOs (sine / tri / saw / square / S&H, free rate or tempo-synced from 16 bar down to 1/64) and 2 envelope followers. Drag a source handle (LFO 1,
LFO 2, ENV 1, ENV 2) onto any knob to assign it. A coloured chip appears under that knob: drag it up/right for more,
down/left for less (negative inverts), double-click or right-click it to remove. The knob ring shows the modulation
range and a live marker.

**3D Granular Field** - the top view shows the grain cloud (X = stereo position, Y = loudness, Z = time). It runs from
the moment the plugin opens (simulated from your settings while there is no audio) and grows brighter and taller with
the incoming sound. Drag to orbit, double-click to reset.

**Presets** - 30 factory presets and user presets (SAVE / DELETE). A preset stores every parameter and the full
modulation routing. User presets are XML files (`.gdpreset`) in `%APPDATA%\AronH\GrannyDelay\Presets` (Windows) or
`~/Library/Application Support/AronH/GrannyDelay/Presets` (macOS).

**Window** - resizable (drag the bottom-right corner); the whole UI scales.

Author: Aron H (clickable link in the plugin header).
