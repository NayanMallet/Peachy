<div align="center"><img src="assets/logotype.png" width="2760" alt="Peachy Logotype"></div>

<div align="center">

### <span style="color:#ff7e5f;">The vocal processor that sounds like hardware, not homework.</span>

*Real-time pitch &amp; formant shifting · hard-tune · robot · analog-style tube drive.*

<br/>

[![Build & Test](https://img.shields.io/badge/build-passing-ff9e6d?style=for-the-badge)](../../actions)
[![Tests](https://img.shields.io/badge/tests-13%2F13-ff7e5f?style=for-the-badge)](tests/)
[![Platform](https://img.shields.io/badge/mac%20·%20win%20·%20linux-1d1e22?style=for-the-badge)](#-installation)
[![Formats](https://img.shields.io/badge/VST3%20·%20AU%20·%20CLAP%20·%20Standalone-1d1e22?style=for-the-badge)](#-installation)
[![License](https://img.shields.io/badge/license-see%20LICENSE-a06b4a?style=for-the-badge)](LICENSE)

<br/>

<img src="assets/manual_overview.png" alt="Peachy interface" width="720"/>

</div>

---

## 🎤 What is Peachy?

**Peachy** is a monophonic, real-time vocal processor built with [JUCE](https://juce.com).
It shifts pitch and formants **independently**, hard-tunes, robotises, and colours
the result with a warm tube drive — all from one skeuomorphic front panel.

It is **not** a generic time-stretcher. Peachy's engine is **TD-PSOLA**
(Time-Domain Pitch-Synchronous Overlap-Add), locked to a real-time pitch tracker,
so it stays smooth and phase-coherent where phase-vocoder tools smear. At neutral
settings it's mathematically transparent; pushed, it becomes an instrument.

---

## ✨ Features

- 🎚️ **Real-time TD-PSOLA engine** — pitch-synchronous, phase-locked, *transparent at unity* (verified to ~4 × 10⁻⁸ error).
- 🗣️ **Independent pitch &amp; formant** — change the note without the character, or the character without the note.
- 🎯 **Three modes** — **Transpose** (clean shift), **Quantize** (instant hard-tune), **Robot** (locked machine voice).
- 🔥 **Asymmetric Tube Drive** — analog-modelled triode saturation with dynamic bias, 4× oversampling and automatic RMS make-up gain.
- 🎛️ **Skeuomorphic UI** — knurled metal knobs, neon indicators, an amber LCD with `<` `>` preset stepping and a random **dice**.
- 🔗 **Bidirectional Link** — tie formant to pitch for chipmunk / monster effects.
- 💾 **20 factory presets** + editable value fields + full DAW automation.
- 🖥️ **Cross-platform** — macOS (Intel &amp; Apple Silicon), Windows, Linux.

---

## 📦 Installation

Grab the latest build from the [**Releases**](../../releases) page and drop it in
your plugin folder:

| Format | macOS | Windows |
|---|---|---|
| **VST3** | `~/Library/Audio/Plug-Ins/VST3/` | `C:\Program Files\Common Files\VST3\` |
| **AU** | `~/Library/Audio/Plug-Ins/Components/` | — |
| **CLAP** | `~/Library/Audio/Plug-Ins/CLAP/` | `C:\Program Files\Common Files\CLAP\` |
| **Standalone** | copy the `.app` anywhere | copy the `.exe` anywhere |

Rescan plugins in your DAW and load **Peachy** on a **mono lead vocal** track.
📖 See the [**User Manual**](docs/MANUAL.md) to get the best out of it.

---

## 🛠️ Build from Source

Peachy builds with **CMake** and **C++23**. JUCE and the other dependencies are
git submodules — clone recursively.

```bash
# 1. Clone with all submodules (JUCE, CLAP, melatonin_blur, ...)
git clone --recursive https://github.com/NayanMallet/Peachy.git
cd Peachy

# ...or, if you already cloned without --recursive:
git submodule update --init --recursive

# 2. Configure (Ninja recommended)
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build Builds --config Release

# 4. Run the tests
ctest --test-dir Builds --output-on-failure
```

**Universal binary (macOS):** add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` at
configure time.

Build artifacts land in `Builds/Peachy_artefacts/`.

<details>
<summary><strong>Dependencies (all vendored as submodules)</strong></summary>

- [JUCE](https://github.com/juce-framework/JUCE) — framework
- [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) — CLAP format
- [melatonin_blur](https://github.com/sudara/melatonin_blur) — drop shadows &amp; glows
- [melatonin_inspector](https://github.com/sudara/melatonin_inspector) — UI debugging
- Catch2 (fetched via CPM) — tests

</details>

---

## 🗺️ Project Layout

```
source/
├── PluginProcessor.{h,cpp}   APVTS, presets, DSP chain
├── PluginEditor.{h,cpp}      3-panel UI, clickable LCD, bidirectional Link
├── PeachyLookAndFeel.h       palette, custom fonts, knobs, halos, LCD menus
└── dsp/
    ├── PitchDetector.*        YIN fundamental tracking
    ├── PitchFormantEngine.*   TD-PSOLA pitch/formant shifter
    └── TubeSaturator.*        Decapitator-style tube drive
docs/     MANUAL.md · ROADMAP.md
tests/    Catch2 unit + DSP + snapshot tests
```

The 3-month evolution plan (MIDI/vocoder control, scale-aware quantize, user
presets, resizable UI) lives in [`docs/ROADMAP.md`](docs/ROADMAP.md).

---

<div align="center">

<div align="center"><img src="assets/logotype.png" width="400" alt="Peachy Logotype"></div>

*Derived from the [Pamplejuce](https://github.com/sudara/pamplejuce) template · Built with JUCE.*

<sub style="color:#5c5e65;">“It's peachy.”</sub>

</div>
