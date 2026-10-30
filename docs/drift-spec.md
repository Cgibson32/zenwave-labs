# Drift — Lo-Fi Space Echo (free)

**Role in the catalog:** the attraction offer. Free with email signup; the plugin that puts the ZenWave name in 50,000 DAWs. It must sound better than "free" and look like the paid family.

## One-line pitch
A tape echo that drifts into deep space: wobbly, saturated repeats that bloom into an endless reverb, with a Freeze button for instant pads and risers.

## Who it is for
Bass and electronic producers first (wonky, halftime, riddim, melodic bass, deep dub), then anyone who wants character delay on vocals, keys and drums.

## Signal flow
```
in → pre-HP (80 Hz) → [+ feedback] → tape delay (wow/flutter) → AGE (LP · hiss · tape sat) → repeats
repeats → SPACE send → SpaceReverb (8-line FDN, Householder, damped, modulated) → + repeats → WIDTH (M/S) → MIX → OUTPUT
```
Feedback is soft-limited (tanh) so settings above 100 % bloom into self-oscillation without clipping the host. The reverb loop is highpassed at 90 Hz so tails never muddy the sub.

## Controls

**Macro row (easy):**

| Control | Range | What it does |
| --- | --- | --- |
| Time | 20–2000 ms, or synced 1/32–1/1 incl. dotted/triplet | Delay time; smoothed over 120 ms so changes pitch-bend like tape |
| Feedback | 0–110 % | Repeat level; >100 % self-oscillates, soft-limited |
| Wobble | 0–100 % | Wow (0.45 Hz), flutter (6.3 Hz) and random drift, up to ~4 ms |
| Age | 0–100 % | Darkens (16 kHz → 1.8 kHz), adds signal-gated hiss and tape saturation |
| Space | 0–100 % | Send from the repeats into the reverb |
| Mix | 0–100 % | Equal-power dry/wet |

**Deep control:**

| Control | Range | What it does |
| --- | --- | --- |
| Size | 0–100 % | Reverb delay lengths (room → cathedral → void) |
| Decay | 0–100 % | Reverb feedback (0.3 → 0.99) |
| Width | 0–150 % | Mid/side width of the wet signal |
| Spread | −100…+100 % | L/R delay time offset (±25 %) |
| Output | −24…+12 dB | Trim |
| Sync | on/off | Tempo sync with host BPM |
| Ping-Pong | on/off | Cross-feed L↔R |
| Freeze | on/off | Stops input, holds delay and reverb forever |

## Quality bar
- 64-bit internal math where it matters (filters, FDN), float I/O
- No zipper noise: every continuous parameter smoothed (30 ms; Time 120 ms)
- Stable at every extreme: verified by `tests/dsp_tests.cpp` (110 % feedback + max age + max space stays bounded; freeze holds; silence in → silence out)
- Sub-safe: 80 Hz pre-HP into the delay, 60 Hz HP after aging, 90 Hz HP in the reverb loop
- Tail length reported to hosts: 20 s
- Mono and stereo buses; mono-in/stereo-out supported

## Presets (27)
Init · Bass (7): Wonky Throw, Halftime Smear, Riddim Slap, Deep Dub Chamber, Growl Tail, Pre-Drop Vacuum, Snare Throw (Dotted) · Space (5): Orbit Decay, Event Horizon, Nebula Wash, Satellite Ping, Cold Vacuum (Dark) · Tape (5): Worn Cassette, Dusty Slapback, Broken Walkman, Vocal Tape Echo, Hazy Keys · Rhythmic (4): 1/8 Pong Tight, Dotted Lead Bounce, Triplet Shuffle, Quarter Note Canyon · FX (5): Self-Oscillate Riser, Infinite Freeze Pad, Transition Sweep, Impact Tail, Alien Transmission

Target before release: 60+ presets, half of them made by Corey in real sessions.

## UI
800×500, resizable (1.6:1). Deep-space black, electric violet accent, starfield, six big macro knobs, a "Deep control" row, preset browser grouped by category, Freeze top-right. One screen, no menus.

## Formats
VST3 (Mac universal, Windows), AU (Mac), Standalone for testing. AAX later.

## Release checklist
- [ ] 60+ presets, auditioned by Corey on a bass-heavy test session
- [ ] Null/measurement pass: aliasing check on Age at 100 %, loudness match at Mix 50 %
- [ ] 2 Mac + 2 Windows beta testers, Ableton and FL Studio
- [ ] Signed + notarized installers from CI
- [ ] Email-gated download page live, 5-email welcome sequence connected
- [ ] Submitted to KVR, Bedroom Producers Blog, Plugin Boutique (free), Reddit, Ableton forum
