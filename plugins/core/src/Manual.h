#pragma once
#include <zenwave_ui/Manual.h>

namespace CoreManual
{
    inline std::vector<zwui::ManualSection> sections()
    {
        using S = zwui::ManualSection;
        std::vector<S> v;
        v.push_back({ "QUICK START", "Core is a three-band saturator built for bass music: the sub stays clean, every band gets its own character, and auto-gain means louder never fools you.",
            { { "1", "Pick a preset. Bass = growls, riddim, neuro and halftime heat; Vocals, Drums and Master = everyday warmth and glue; FX = folders, crushers and broken speakers." },
              { "2", "Turn DRIVE until it bites, then HEAT for warmth and presence. Auto-gain holds the level, so judge by tone, not loudness." },
              { "3", "Leave SUB LOCK on for bass. Drag the two lines in the spectrum (or press LEARN) to put the band splits where your track needs them." },
              { "4", "Pick a character per band in the BANDS card. Tape on the low mids, Tube on the body, Fold or Rectify on the highs is the classic bass-music recipe." } } });

        v.push_back({ "HEAT", "The six big knobs.",
            { { "DRIVE", "Master amount of saturation; scales all three band drives together. Auto-gain compensates, so the level barely moves while the harmonics pile up.", "drive" },
              { "HEAT", "Character of the distortion: adds asymmetry (even harmonics, tube-like warmth) and pushes presence into the shaper so it bites instead of just squashing.", "heat" },
              { "FOCUS", "Mid/side focus. Left = distort only the centre (bass, kick, vocal) and leave the sides clean. Right = distort only the sides (width, air) and keep the centre tight. Centre = both.", "focus" },
              { "WIDTH", "Stereo width of the wet signal. 100 % = natural, up to 150 % = extra wide, 0 = mono.", "width" },
              { "MIX", "Parallel blend. 100 % = fully processed; 30-50 % is the secret to parallel grit on vocals and drums.", "mix" },
              { "OUTPUT", "Output trim, -24 to +12 dB.", "output" },
              { "SUB LOCK", "Everything below the LOW crossover passes clean and mono. Keep it on for anything with a sub. Off = the low band gets the LOW character too (tape on an 808 is lovely)." },
              { "BIT CRUSHER", "A crusher on the summed wet signal (the locked sub stays clean). CRUSH turns it on; BITS = word length from 16 (clean) down to 2 (destroyed); RATE = sample-rate reduction (aliasing, metallic); C.MIX = blend it under the saturation. Modulate BITS with a synced Random LFO for glitch rhythms.", "crushbits" },
              { "RATE", "Sample-rate reduction of the crusher: 0 = full rate, 100 % = a handful of kHz. Aliasing becomes part of the sound.", "crushrate" },
              { "C.MIX", "How much of the crushed signal replaces the clean saturated one.", "crushmix" },
              { "LOW X / HIGH X", "The band split frequencies (also draggable as lines in the spectrum). LOW X 40-400 Hz, HIGH X 1-12 kHz.", "lowx" },
              { "HIGH X", "Upper band split, 1-12 kHz.", "highx" } } });

        v.push_back({ "BANDS", "LOW | MID | HIGH. Each band has a character, its own drive, a parallel mix and a gain.",
            { { "Character", "Tape = smooth, rounded, musical. Tube = warm with even harmonics. Fold = wavefolder, synth-like and gnarly at high drive. Rectify = half-wave fuzz, aggressive and buzzy. Crush = bit and sample-rate reduction (deliberately aliased)." },
              { "DRIVE", "How hard this band hits its shaper, multiplied by the master DRIVE.", "b1drive" },
              { "DRIVE", "How hard this band hits its shaper, multiplied by the master DRIVE.", "b2drive" },
              { "DRIVE", "How hard this band hits its shaper, multiplied by the master DRIVE.", "b3drive" },
              { "MIX", "Parallel blend inside the band - a little clean signal under a heavily driven band keeps transients alive.", "b1mix" },
              { "MIX", "Parallel blend inside the band.", "b2mix" },
              { "MIX", "Parallel blend inside the band.", "b3mix" },
              { "GAIN", "Band level after processing, -18 to +18 dB. Think of it as a three-band tone control that happens to distort.", "b1gain" },
              { "GAIN", "Band level after processing.", "b2gain" },
              { "GAIN", "Band level after processing.", "b3gain" },
              { "AUTO readout", "Shows how much auto-gain is compensating in each band. Big numbers mean you're driving that band hard." },
              { "Band splits", "Drag the two glowing lines in the spectrum to move the LOW | MID | HIGH crossovers (40-400 Hz and 1-12 kHz). They're Linkwitz-Riley, so the bands sum perfectly flat when nothing is driven." } } });

        {
            auto f = zwui::fieldSection();
            f.entries.insert(f.entries.begin(), { "PERFORM PAD", "The throw surface. Drag: across = DRIVE, up = MIX. Let go and it glides back to your knob settings over the GLIDE division - a perfectly timed distortion swell. Recorded as Pad X / Pad Y / Pad Hold automation; map a controller pad to Pad Hold." });
            f.entries.insert(f.entries.begin(), { "LEARN", "Play the track and press LEARN. For six seconds Core listens to the dry input, then puts the LOW split above your sub fundamental and the HIGH split under your top end, so the three bands fit this track." });
            v.push_back(f);
        }
        v.push_back(zwui::motionSection());
        v.push_back(zwui::presetSection());
        v.push_back({ "RECIPES", "",
            { { "Growl that bites on hits", "'Growl Glue' - the envelope follower is already pushing MID DRIVE. Raise ROUTING slot 1 for more bite on every hit." },
              { "Parallel vocal grit", "'Grit Vox (Parallel)' - FOCUS left so only the centre distorts, MIX 35 %. The vocal keeps its air and gains chest." },
              { "Drum bus glue", "'Drum Bus Glue' - Tape on all three, DRIVE 30 %, HEAT 35 %. Nothing obvious, everything louder-feeling at the same level." },
              { "Sub-safe smash", "'Sub-Safe Smash' - mids and highs at 95 % drive, SUB LOCK on. The drop stays clean under total destruction." } } });
        return v;
    }
}
