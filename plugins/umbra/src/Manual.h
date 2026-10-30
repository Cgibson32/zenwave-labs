#pragma once
#include <zenwave_ui/Manual.h>

namespace UmbraManual
{
    inline std::vector<zwui::ManualSection> sections()
    {
        using S = zwui::ManualSection;
        std::vector<S> v;
        v.push_back({ "QUICK START", "Umbra is a dark reverb built for bass music: the sub never goes in, the tail can duck out of the way, and loud hits can bloom.",
            { { "1", "Pick a preset. Bass = drop-safe rooms and throws, Dark = the signature deep spaces, Mix = everyday glue and halos, FX = freezes, impacts and swells." },
              { "2", "Set SIZE and DECAY for the space, DARK for how much light gets in." },
              { "3", "GRAVITY keeps the low end dry (raise it until the sub stays clean). DUCK pushes the tail down while the source plays." },
              { "4", "MIX blends it in. On a send run 100 % and use the EQ in the Field card to carve the tail." } } });

        v.push_back({ "SPACE", "The five big knobs.",
            { { "PRE-DELAY", "Gap between the dry sound and the reverb, 0-250 ms. 20-40 ms keeps vocals and leads upfront; 0 glues; long values feel like a slap into the space.", "predelay" },
              { "SIZE", "Dimensions of the space, from a tight chamber to a cavern. Changing it while playing smears pitch a little - that's the modulation keeping it smooth.", "size" },
              { "DECAY", "Tail length, 0.4 s to about 24 s.", "decay" },
              { "DARK", "Umbra's signature tone control. Left = open and airy, right = pitch black: highs die fast, lows and low mids linger.", "dark" },
              { "MIX", "Dry/wet balance. The dry path is untouched.", "mix" },
              { "SYNC + division / MOTION RATE", "The internal modulation that keeps the tail alive can run free (MOTION RATE multiplier) or lock to the tempo so the slow swirl completes once per division." },
              { "ORBIT", "Links this plugin with every other ZenWave plugin on your system that has Orbit on. Freeze one and they all freeze together (Drift's loop and Umbra's tail lock at the same instant). The button shows how many are in orbit." },
              { "FREEZE", "Holds the current tail forever as a clean, infinite pad. Automate it for breakdowns; sweep DARK and the filters while frozen." } } });

        v.push_back({ "SHAPE", "Fine control of the tail's behaviour.",
            { { "BLOOM", "Loud input temporarily opens the decay - hits grow a longer tail, quiet parts stay tight. The 'opening' sound of the Signature preset.", "bloom" },
              { "DENSITY", "Diffusion. Low = you can hear individual reflections (good for percussive, grainy spaces); high = smooth, dense wash.", "density" },
              { "MOTION", "Depth of the internal modulation. More = richer, chorus-like tail that never rings on one pitch; less = static and precise.", "motion" },
              { "MOTION RATE", "Speed multiplier for that modulation (0.1-4x). Low = slow swirl, high = shimmer.", "motionrate" },
              { "GRAVITY", "Crossover from 40 to 300 Hz. Everything below it bypasses the reverb and stays dry and mono - this is what keeps drops clean. Raise it until the sub is untouched.", "gravity" },
              { "EARLY", "Level of early reflections - the first few bounces. More = a sense of walls and placement; less = pure tail.", "early" },
              { "DUCK", "Sidechains the wet signal to the dry input. While the source plays the tail drops back; when it stops, the space swells up behind it. Great on vocals and leads.", "duck" },
              { "WIDTH", "Stereo width of the wet signal. 100 % = natural, up to 150 % = extra wide, 0 = mono.", "width" },
              { "OUTPUT", "Output trim, -24 to +12 dB.", "output" } } });

        {
            auto f = zwui::fieldSection();
            f.entries.insert(f.entries.begin(), { "PERFORM PAD", "The throw surface. Drag: across = decay, up = send (mix). Let go and the space glides back to your knob settings over the GLIDE division - swell a word or a snare into a cathedral and have it fall away in time. Recorded as Pad X / Pad Y / Pad Hold automation; map a controller pad to Pad Hold." });
            f.entries.insert(f.entries.begin(), { "LEARN", "Play the track and press LEARN. For six seconds the plugin listens to the dry input, then places the wet Low Cut just above your sub fundamental, the High Cut where your top end actually ends, and Gravity so the sub stays dry - the effect sits in *this* track instead of a generic one. Fine-tune from there." });
            v.push_back(f);
        }
        v.push_back(zwui::motionSection());
        v.push_back(zwui::presetSection());
        v.push_back({ "RECIPES", "",
            { { "Drop-safe snare space", "'Snare Dungeon' - GRAVITY around 120 Hz, DUCK 30 %, HIGH CUT 6 kHz. The tail sits behind the hit and never muddies the sub." },
              { "Vocal halo", "'Lead Halo (Ducked)' - PRE-DELAY 30 ms, DUCK 45 %, DARK 60 %. The vocal stays dry while it sings and glows when it rests." },
              { "Black hole build", "'Pre-Drop Black Hole' - env follower is already pushing DECAY and SIZE. Automate FREEZE on the last beat and cut the dry." },
              { "Living pad", "'Collapsed Star' - a slow Drift LFO moves PRE-DELAY and DARK so the space breathes." } } });
        return v;
    }
}
