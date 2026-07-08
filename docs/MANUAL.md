<div align="center">

<img src="assets/logo.png" alt="Peachy" width="360"/>

<h1 style="color:#ff9e6d; letter-spacing:2px; margin-bottom:0;">PEACHY</h1>
<p style="color:#ff7e5f; font-size:18px; margin-top:4px;"><strong>Vocal Processor — User Manual</strong></p>

<sub>Pitch · Formant · Hard-Tune · Robot · Tube Drive</sub>

</div>

---

<h2 style="color:#ff9e6d;">🍑 The Basics</h2>

![Peachy — full interface](assets/manual_overview.png)

Peachy is a **monophonic, real-time vocal processor**. It shifts pitch and formants
independently, hard-tunes, robotises, and runs the whole thing through a warm
tube-style drive — from a single, obvious front panel.

Under the hood it is *not* a generic time-stretcher. Peachy uses a **TD-PSOLA**
engine (Time-Domain Pitch-Synchronous Overlap-Add) locked to the pitch detected in
real time. That is what gives it a smooth, analog-leaning grain instead of the
smeared, granular artifacts you get from phase-vocoder tools. At neutral settings
Peachy is *mathematically transparent* — it hands your voice straight back.

> <strong style="color:#ff9e6d;">The golden rule.</strong> Peachy is built for
> <strong>one clean, dry, monophonic voice</strong> — the same discipline the classic
> hardware pitch boxes demanded. No chords, no stacked harmonies, no reverb on the
> way in. Put Peachy <strong>first in the insert chain</strong>, before compression
> and before any time-based effects.

<table>
<tr><td style="background:#1d1e22; color:#ff9e6d; padding:10px 14px;">
<strong>Do</strong> — solo lead vocal · dry &amp; close-mic'd · clean/gated before Peachy
</td></tr>
<tr><td style="background:#1d1e22; color:#a06b4a; padding:10px 14px;">
<strong>Avoid</strong> — polyphonic material · heavy room / reverb · noisy backgrounds
</td></tr>
</table>

---

<h2 style="color:#ff9e6d;">🎛 Control Panel</h2>

Peachy's panel splits into three sections — **Pitch &amp; Formant** (left),
**Mode** (centre) and **Color &amp; Output** (right) — with a clickable LCD in the
header for presets.

<br/>

<h3 style="color:#ff7e5f;">PITCH &amp; FORMANT</h3>

![Pitch & Formant knobs](assets/manual_pitch_formant.png)

**PITCH** &nbsp;·&nbsp; `−12 … +12 semitones`
Transposes the voice. Because formants are preserved, shifting an octave changes
the *note*, not the *character* — the singer still sounds like themselves.

- `±2–3 st` — subtle harmonies and thickening
- `+12 / −12` — parallel octaves (drop **MIX** to ~40–50 %)
- In **ROBOT** mode, PITCH becomes the offset of the locked target note.

**FORMANT** &nbsp;·&nbsp; `−12 … +12 semitones`
Moves the spectral envelope — the perceived *size* of the voice — **without**
changing the note. The most musical control on the panel:

- `+2 … +4` — younger, lighter, brighter
- `−2 … −4` — bigger, darker, chestier
- extremes — cartoon chipmunk or movie-monster

<br/>

<h3 style="color:#ff7e5f;">LINK</h3>

![Link toggle](assets/manual_link.png)

**LINK** ties FORMANT to PITCH (and back — the link is **bidirectional**). Turn it
on for "sped-up tape" effects, where pitch and timbre must move together
(chipmunk / monster). Leave it **off** for everything else — the whole point of
Peachy is that pitch and formant stay independent.

<br/>

<h3 style="color:#ff7e5f;">MODE</h3>

![Mode buttons](assets/manual_mode.png)

<table>
<tr>
  <td style="background:#1d1e22; color:#ff9e6d; padding:10px 14px; white-space:nowrap;"><strong>TRANSPOSE</strong></td>
  <td style="background:#1d1e22; color:#c9cbd1; padding:10px 14px;">The pure shifter. PITCH and FORMANT drive the engine directly. The default, "musical" mode.</td>
</tr>
<tr>
  <td style="background:#1d1e22; color:#ff9e6d; padding:10px 14px; white-space:nowrap;"><strong>QUANTIZE</strong></td>
  <td style="background:#1d1e22; color:#c9cbd1; padding:10px 14px;">Hard-tune. Every sung note snaps to the nearest semitone, almost instantly — the pop / hip-hop signature. PITCH adds an offset <em>after</em> correction (e.g. +12 for octave hard-tune).</td>
</tr>
<tr>
  <td style="background:#1d1e22; color:#ff9e6d; padding:10px 14px; white-space:nowrap;"><strong>ROBOT</strong></td>
  <td style="background:#1d1e22; color:#c9cbd1; padding:10px 14px;">Kills the melody. The voice is locked to one fixed pitch (C3 + PITCH). Vibrato disappears, diction stays — the classic machine voice.</td>
</tr>
</table>

<br/>

<h3 style="color:#ff7e5f;">DRIVE</h3>

![Drive knob](assets/manual_drive.png)

**DRIVE** &nbsp;·&nbsp; `0 … 100 %`
Tube-style saturation, modelled after driven triode input stages: asymmetric,
envelope-reactive (the bias *breathes* with the vocal), with an upper-mid tilt
that keeps it aggressive and warm at once. Output level is **auto-compensated** —
push the drive and the perceived loudness stays put while harmonic density climbs.
At 100 % it is filthy, on purpose.

<br/>

<h3 style="color:#ff7e5f;">MIX</h3>

![Mix knob](assets/manual_mix.png)

**MIX** &nbsp;·&nbsp; `0 … 100 %`
Wet/dry balance, **latency-compensated** (no comb filtering).

- `100 %` — the effect replaces the voice (hard-tune, robot)
- `30–50 %` — doublers, octaves, parallel saturation

<br/>

<h3 style="color:#ff7e5f;">The LCD &amp; Presets</h3>

![Preset LCD](assets/manual_lcd.png)

The header LCD shows the current **preset** (top line) and **mode** (bottom line).
Click the text to open the preset list; use **`<`** / **`>`** to step through
presets; hit the **dice** to roll fresh, musical random settings. Every value chip
under a knob is editable — double-click, type, Enter.

---

<h2 style="color:#ff9e6d;">⚡ Quick Recipes</h2>

<h3 style="color:#ff7e5f;">Modern Hard-Tune</h3>

> <code style="color:#ffb98a;">MODE = QUANTIZE · MIX 100% · DRIVE 10–20% · PITCH 0</code>

Sing legato, aim at the notes, and let the snapping do the work. Too much effect?
That *is* the effect. Add a touch of DRIVE for edge.

<h3 style="color:#ff7e5f;">Robot / Machine Voice</h3>

> <code style="color:#ffb98a;">MODE = ROBOT · PITCH −5 … 0 · DRIVE 20–40% · MIX 100%</code>

Locks the voice to one note. Automate **MIX** from 0 → 100 % to make the machine
rise into a phrase. Lower PITCH for a deeper droid, raise it for a chirpy one.

<h3 style="color:#ff7e5f;">Natural Doubler</h3>

> <code style="color:#ffb98a;">MODE = TRANSPOSE · PITCH +0.2 · FORMANT −0.8 · MIX ~45%</code>

A tiny detune against the dry signal creates a gentle, believable double.
Duplicate the track and pan hard for a wide stereo double.

<h3 style="color:#ff7e5f;">Gender / Character Shift</h3>

> <code style="color:#ffb98a;">MODE = TRANSPOSE · PITCH ±3 · FORMANT ±3 · LINK off</code>

Move PITCH and FORMANT together for a full, natural gender flip; move **only**
FORMANT to change the *character* while keeping the melody intact.

---

<h2 style="color:#ff9e6d;">📋 Specs &amp; Support</h2>

<table>
<tr><td style="background:#1d1e22; color:#ff9e6d; padding:8px 14px;"><strong>Formats</strong></td>
    <td style="background:#1d1e22; color:#c9cbd1; padding:8px 14px;">VST3 · AU · AUv3 · CLAP · Standalone</td></tr>
<tr><td style="background:#1d1e22; color:#ff9e6d; padding:8px 14px;"><strong>Platforms</strong></td>
    <td style="background:#1d1e22; color:#c9cbd1; padding:8px 14px;">macOS (Intel &amp; Apple Silicon) · Windows · Linux</td></tr>
<tr><td style="background:#1d1e22; color:#ff9e6d; padding:8px 14px;"><strong>Sample rates</strong></td>
    <td style="background:#1d1e22; color:#c9cbd1; padding:8px 14px;">44.1 – 192 kHz</td></tr>
<tr><td style="background:#1d1e22; color:#ff9e6d; padding:8px 14px;"><strong>Latency</strong></td>
    <td style="background:#1d1e22; color:#c9cbd1; padding:8px 14px;">~50 ms, reported to the host for automatic delay compensation</td></tr>
<tr><td style="background:#1d1e22; color:#ff9e6d; padding:8px 14px;"><strong>Detection range</strong></td>
    <td style="background:#1d1e22; color:#c9cbd1; padding:8px 14px;">60 Hz – 1 kHz (mono voice)</td></tr>
<tr><td style="background:#1d1e22; color:#ff9e6d; padding:8px 14px;"><strong>Channels</strong></td>
    <td style="background:#1d1e22; color:#c9cbd1; padding:8px 14px;">Mono or stereo in; processing is monophonic, output mirrored to both channels</td></tr>
</table>

**Live tracking?** Peachy's ~50 ms of look-ahead is normal for a formant-correct
pitch engine. For live takes, monitor through your interface's direct path and let
the DAW compensate on playback.

<br/>

<div align="center">
<sub style="color:#5c5e65;">Peachy Audio · Built with JUCE · “It's peachy.”</sub>
</div>
