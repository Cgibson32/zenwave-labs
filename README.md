# ZenWave Labs — plugins monorepo

Premium audio plugins for bass and electronic producers. Space-themed, flat-priced, built on JUCE 8 with a shared, JUCE-free DSP library.

```
zenwave-labs/
├── libs/zenwave_dsp/      Header-only DSP: delay, filters, saturation, SpaceReverb, DriftEngine
├── plugins/drift/         Drift — lo-fi space echo (free). First product.
├── tests/                 DSP unit tests (no JUCE required)
├── docs/                  Product specs
├── tools/                 Signing / notarization scripts
└── .github/workflows/     CI: Linux tests, macOS (VST3+AU, universal) and Windows (VST3) builds
```

## Build

Requirements: CMake 3.22+, a C++20 compiler. JUCE is fetched automatically on first configure.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Drift_VST3 Drift_Standalone --parallel
# macOS additionally:
cmake --build build --target Drift_AU --parallel
```

Built plugins land in `build/plugins/drift/Drift_artefacts/Release/`. Set `-DZENWAVE_COPY_PLUGINS=ON` to copy them into your user plugin folders after each build.

Linux needs the usual JUCE dev packages (`libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev libcurl4-openssl-dev libgtk-3-dev libwebkit2gtk-4.1-dev`).

## Test the DSP without a DAW

```bash
g++ -std=c++20 -O2 -Ilibs/zenwave_dsp/include tests/dsp_tests.cpp -o dsp_tests && ./dsp_tests
```

## Releasing

Push a tag like `v0.1.0`. CI builds macOS and Windows, signs and notarizes the Mac bundles when the Apple secrets are configured, and attaches zips to the GitHub release.

Secrets to add once (Settings → Secrets → Actions): `APPLE_CERT_P12` (base64 of the Developer ID .p12), `APPLE_CERT_PASSWORD`, `APPLE_ID`, `APPLE_TEAM_ID`, `APPLE_APP_PASSWORD`. Windows signing is added the same way when the certificate exists.

## Product roadmap

Drift (free) → Umbra (dark reverb, $49) → Halo (shimmer reverb, $49) → Core (multiband saturator, $59) → Nebula (pad synth, $99) → Horizon (AI mastering, $129) → Siren → Xeno → Stellar Strings. Shared code lives in `libs/zenwave_dsp`; Umbra and Halo start from `SpaceReverb`.

## House rules

- One screen per plugin. Macro row for speed, deep row for control.
- Every continuous parameter is smoothed. Every extreme setting is tested for stability.
- Sub-safe by default: nothing below 60–90 Hz goes into feedback or reverb loops.
- Flat prices. No sales. New releases are the only scarcity.

## Motion & presets (v0.4)

- **MOTION card** — two LFOs (sine / triangle / saw up / saw down / square / random / drift; free-running or tempo-synced and phase-locked to the host) and an envelope follower (attack / release / sensitivity), routed through four slots to any continuous knob. Modulated knobs show a range arc and a live cyan dot.
- **Presets** — ‹ › browse, **SAVE** writes a user preset to `Documents/ZenWave Labs/<Plugin>/Presets/`, **A | B** compare (right-click copies the current slot to the other), **WARP** randomises (gentle; Shift = bolder; Cmd/Ctrl = anywhere). Right-click a knob to lock it from Warp or reset it.
- **Field** — real-time spectrum (4096-pt FFT, 160 log bins, peak hold, dry-input ghost) with the EQ response and cutoff handles overlaid.

## Perform, Orbit, Learn (v0.5)

- **Perform pad** (Field card) - drag to throw (Drift: time x feedback, snapped to divisions when synced; Umbra: decay x send), release and it glides home over a synced division. Pad X / Y / Hold are automatable and MIDI-mappable.
- **ORBIT** - every ZenWave plugin with Orbit on shares one Freeze over a shared-memory bus; the button shows the peer count.
- **LEARN** - listens to the dry input for 6 s and places Low Cut above your sub, High Cut where the top end ends (Umbra also sets Gravity).
- Manual lives in the MANUAL chip, top right; everything has a tooltip.

## CORE - multiband heat (v0.6)

Three-band saturator for bass music. LR4 crossovers (drag the lines in the spectrum, or LEARN), per-band character (Tape / Tube / Fold / Rectify / Crush) with drive, mix and gain, SUB LOCK keeps the low band clean and mono, FOCUS distorts mid or side only, auto-gain per band so louder never fools you. 4x oversampled shapers, same shell as Drift/Umbra (presets, Warp, Motion matrix, perform pad = drive x mix, manual, tooltips). 22 presets.

## PULSE - multiband dynamics (v0.7)

Three-band upward/downward compressor (the OTT idea) with adaptive thresholds (AUTO), 3 ms lookahead, program-dependent release, transient shaper (ATTACK / SUSTAIN), pattern duck (Steps LFO -> DUCK) and external sidechain, GRIT, and a four-node dynamic EQ drawn on the spectrum. MATCH places the splits and trims output so out = in. 19 presets, same shell as the rest of the line.

## NEBULA - planetary wavetable synth (v0.9.1)

Two oscillators on eight procedurally generated, band-limited wavetables (Glass, Voice, Metal, Grain, Dune, Halo, Ion, Void), 32 frames each. The planet centrepiece draws oscillator A as a sphere of rings, B as its orbit, playing voices as moons and the harmonic spectrum as a halo. CARVE: up to 8 hand-placed harmonic bands (centre, width, gain) rendered into the wavetable. Per-voice SVF filter + drive, two ADSRs, unison with mono-safe spread and drift, Umbra's reverb as SPACE, sidechain DUCK so pads sit behind the track, eight macros, Motion matrix, on-screen keyboard, 14 presets.

**FX page** (VOICE / FX tabs over the middle band): the patch's own processing chain, in order EQ -> SATURATE -> GLUE -> ECHO -> SPACE. EQ: four nodes (bell / shelves / cuts) placed by clicking on a live spectrum of the synth's output; node gains are modulation targets. SATURATE runs Core's eight waveshapers (Tape, Tube, Fold, Rectify, Crush, Diode, Octave, Hard) 2x oversampled with DRIVE / TONE / BIAS / LOW (keep-lows crossover) / OUT / MIX. GLUE is a single-band Pulse compressor (upward + downward, adaptive) with UP / DOWN balance, parallel MIX, OUT and a live GR readout. ECHO is a tempo-synced ping-pong delay with a filtered, saturating feedback loop plus WOBBLE (tape wow) and WIDTH. SPACE is Umbra's reverb with SIZE / DECAY / MIX / DARK / PRE-delay / LOW CUT and a SHIMMER (octave-up feedback) switch; its stereo width rides the WIDTH macro. Every module has an on/off, every knob is modulatable, everything saves with the preset.
