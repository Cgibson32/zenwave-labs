#pragma once
#include <zenwave_ui/Manual.h>

namespace DriftManual
{
    inline std::vector<zwui::ManualSection> sections()
    {
        using S = zwui::ManualSection;
        std::vector<S> v;
        v.push_back({ "QUICK START", "Drift is a lo-fi space echo: a tape delay with wobble, age and a reverb tail built in.",
            { { "1", "Pick a preset. Categories: Bass (throws, halftime, dub), Space (pads, intros), Tape (lo-fi, slapback), Rhythmic (synced, mix-ready), FX (risers, freezes)." },
              { "2", "Set TIME (or SYNC + a division) and FEEDBACK for how many repeats you want." },
              { "3", "Dial character with WOBBLE (pitch movement), AGE (tape saturation and darkening) and SPACE (reverb on the tail)." },
              { "4", "MIX blends it in. On a send, run MIX at 100 %. For throws, automate MIX or FREEZE." } } });

        v.push_back({ "ECHO", "The main delay controls - the six big knobs.",
            { { "TIME", "Delay time, 20 ms - 2 s. Short (60-120 ms) = slapback and thickening; 1/8 and dotted 1/8 = classic bass-music throws; long = ambient washes. When SYNC is on this knob is bypassed and the division sets the time.", "time" },
              { "SYNC + division", "Locks the delay to host tempo. 1/8. (dotted) and 1/4T are the ones everybody reaches for; 2 and 4 bars make rhythmic pads out of single hits." },
              { "FEEDBACK", "How much of each repeat goes back in. 30-50 % = a few repeats, 70-90 % = long trails, above 100 % self-oscillates (the loop runs away - ride it with the EQ and MIX for risers).", "feedback" },
              { "WOBBLE", "Wow and flutter: slow and fast pitch drift of the tape. A little (15-30 %) adds life, a lot turns repeats seasick.", "wowflutter" },
              { "Wobble SYNC + division / WOBBLE RATE", "The slow wow component can run free (WOBBLE RATE in Hz) or lock to the tempo so the pitch dip lands on the beat." },
              { "AGE", "Tape wear. Rolls off highs, adds hiss when there's signal, saturates the loop and adds head-bump low mids. Each repeat gets a little older than the last, like real tape.", "age" },
              { "SPACE", "Sends the repeats (and a touch of the input) into the built-in reverb. 0 = dry echo, 100 % = the repeats dissolve into a wash.", "space" },
              { "PING-PONG", "Alternates repeats left/right. Turn off for a centred, mono-compatible echo." },
              { "FREEZE", "Captures what's in the loop and holds it forever, clean. Great for drops and transitions - set it, then sweep the filters. Automate it in your DAW." },
              { "MIX", "Dry/wet balance. The dry path is untouched.", "mix" } } });

        v.push_back({ "DEEP SPACE", "Fine control over the loop's tone, the reverb and the stereo image.",
            { { "COLOR", "Filter inside the feedback loop. Left = Dark (each repeat gets duller - dub style). Right = Thin (each repeat loses low end - telephone / transmission). Centre = off.", "color" },
              { "SIZE", "Size of the reverb space that SPACE feeds. Small = room-ish smear, large = hall.", "size" },
              { "DECAY", "Reverb tail length, 0.4 s up to about 24 s.", "decay" },
              { "WIDTH", "Stereo width of the wet signal. 100 % = natural, above = extra wide (check the correlation ring), 0 = mono.", "width" },
              { "SPREAD", "Offsets left and right delay times. Small amounts thicken; large amounts make lopsided, rolling patterns.", "spread" },
              { "WOBBLE RATE", "Speed of the slow wow (0.05-4 Hz) when wobble sync is off.", "wobblerate" },
              { "OUTPUT", "Output trim, -24 to +12 dB. Use it to match levels when A/B-ing.", "output" } } });

        v.push_back(zwui::fieldSection());
        v.push_back(zwui::motionSection());
        v.push_back(zwui::presetSection());
        v.push_back({ "RECIPES", "",
            { { "Vocal throw", "SYNC 1/8., FEEDBACK 45 %, AGE 55 %, SPACE 35 %, MIX 100 % on a send. Automate the send on the last word of a phrase." },
              { "Pre-drop vacuum", "Load 'Pre-Drop Vacuum', automate FREEZE on at the end of the build and sweep LOW CUT up to 400 Hz over the last bar." },
              { "Dub chamber", "'Deep Dub Chamber' - COLOR to the left so every repeat gets darker, FEEDBACK 70 %, then tap the send on snares." },
              { "Alien transmission", "'Alien Transmission' - a synced Random LFO is already on TIME. Push the amount in ROUTING slot 1 for bigger pitch jumps." } } });
        return v;
    }
}
