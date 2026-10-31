#pragma once
#include <zenwave_ui/Manual.h>

namespace PulseManual
{
    inline std::vector<zwui::ManualSection> sections()
    {
        using S = zwui::ManualSection;
        std::vector<S> v;
        v.push_back({ "QUICK START", "Pulse is a three-band upward/downward compressor (the OTT idea) that adapts to your track, with a transient shaper, a pattern/sidechain duck and a four-node dynamic EQ on top.",
            { { "1", "Pick a preset. Signature = engineered starting points; Bass, Vocals, Drums, Rhythm and Master do what they say." },
              { "2", "DEPTH is the one knob: it squashes peaks down and lifts detail up in every band. TIME sets how fast all of it moves. Leave AUTO on - the thresholds follow the song." },
              { "3", "Press MATCH while the track plays: the band splits land around your sub and top end and OUTPUT is set so loud is not fooling you." },
              { "4", "Hits lost their snap? ATTACK up. Too much tail? SUSTAIN down. Mud or harshness in one spot? Double-click a node on the spectrum and drag it there." } } });

        v.push_back({ "DYNAMICS", "The six big knobs plus the second row.",
            { { "DEPTH", "Master amount of compression. Peaks above each band's reference get pulled down (3:1), quiet detail below it gets lifted (3:1, up to +18 dB). 30-50 % is glue; 80-100 % is the full OTT crush.", "depth" },
              { "TIME", "Scales attack and release in every band from 0.3x (snappy, grabby) to 2x (slow, smooth). Release is also program-dependent: dense passages release faster, sparse hits slower.", "time" },
              { "ATTACK (shape)", "Transient shaper: right adds up to +9 dB of snap to every hit, left softens it. Works after the compression, so you can crush and still punch.", "shapeatk" },
              { "SUSTAIN (shape)", "Right lifts the body between hits (bigger, longer), left pulls it down (tighter, drier - a de-roomer).", "shapesus" },
              { "MIX", "Parallel blend. The dry path is delayed by the same 3 ms lookahead, so blends stay phase-coherent.", "mix" },
              { "OUTPUT", "Output trim. MATCH sets it for you so the result is as loud as the input.", "output" },
              { "AUTO", "Thresholds follow a slow loudness average per band (about 6 s). DEPTH then does the same job on a whisper and a drop. Off = fixed thresholds around -18 dBFS, the classic behaviour that reacts to how hot you drive it (use INPUT)." },
              { "SUB LOCK", "No dynamics below the LOW crossover. The sub stays exactly as you wrote it; band GAIN still applies." },
              { "GRIT", "A gentle tape stage after the dynamics so heavy compression adds warmth instead of flattening." },
              { "INPUT", "Drive into the detectors, -24 to +24 dB. Mostly for AUTO off.", "in" },
              { "DUCK", "Manual duck amount. Route the Steps LFO to it for a synced pump or gate that needs no sidechain routing (see the Pump and Gate presets).", "duck" },
              { "SC AMT / SC REL", "External sidechain: route your kick to Pulse's sidechain input in the DAW, then SC AMT sets how far it ducks and SC REL how fast it comes back. Dims when no sidechain is connected.", "scamount" },
              { "SC REL", "Sidechain release, 20 ms - 1 s.", "screlease" },
              { "LOW X / HIGH X", "Band split frequencies (also draggable in the spectrum).", "lowx" }, { "HIGH X", "Upper split, 1-12 kHz.", "highx" } } });

        v.push_back({ "BANDS + DYNAMIC EQ", "Per band: on/off, solo, DEPTH, GAIN, ATK and REL multipliers, and a live gain meter (magenta = pulling down, teal = lifting up). Below: four dynamic-EQ nodes.",
            { { "DEPTH", "This band's share of the master DEPTH.", "b1depth" }, { "DEPTH", "This band's share of the master DEPTH.", "b2depth" }, { "DEPTH", "This band's share of the master DEPTH.", "b3depth" },
              { "GAIN", "Band level after compression - a three-band tone control.", "b1gain" }, { "GAIN", "Band level after compression.", "b2gain" }, { "GAIN", "Band level after compression.", "b3gain" },
              { "ATK / REL", "Multipliers on the band's base timing (low band is slowest, high fastest).", "b1atk" }, { "ATK", "Attack multiplier.", "b2atk" }, { "ATK", "Attack multiplier.", "b3atk" },
              { "REL", "Release multiplier.", "b1rel" }, { "REL", "Release multiplier.", "b2rel" }, { "REL", "Release multiplier.", "b3rel" },
              { "Dynamic EQ nodes", "Each node is a peaking filter that only moves when its frequency misbehaves. FREQ and WIDTH place it, THRESH is how far above that frequency's own average it has to get, RANGE is how far it moves (negative = cut when loud: mud, harshness, resonances; positive = lift when loud: presence that follows the performance). Drag nodes straight on the spectrum; Shift-drag changes width; double-click toggles." },
              { "FREQ", "Node centre frequency.", "n1freq" }, { "FREQ", "Node centre frequency.", "n2freq" }, { "FREQ", "Node centre frequency.", "n3freq" }, { "FREQ", "Node centre frequency.", "n4freq" },
              { "WIDTH", "Node bandwidth (Q 0.3 wide - 12 surgical).", "n1q" }, { "WIDTH", "Node bandwidth.", "n2q" }, { "WIDTH", "Node bandwidth.", "n3q" }, { "WIDTH", "Node bandwidth.", "n4q" },
              { "THRESH", "How far above its own average the band must get before the node acts (relative, so it works at any level).", "n1thr" }, { "THRESH", "Threshold above the node's average.", "n2thr" }, { "THRESH", "Threshold above the node's average.", "n3thr" }, { "THRESH", "Threshold above the node's average.", "n4thr" },
              { "RANGE", "Maximum gain change, -18 to +18 dB, reached 12 dB past threshold.", "n1range" }, { "RANGE", "Maximum gain change.", "n2range" }, { "RANGE", "Maximum gain change.", "n3range" }, { "RANGE", "Maximum gain change.", "n4range" } } });

        {
            auto f = zwui::fieldSection();
            f.entries.erase(f.entries.begin() + 3, f.entries.end());   // Pulse has no wet-path EQ knobs
            f.entries.insert(f.entries.begin(), { "PERFORM PAD", "Drag: across = DEPTH, up = DUCK. Let go and it glides home over the GLIDE division. Automatable and MIDI-mappable." });
            f.entries.insert(f.entries.begin(), { "MATCH", "Six seconds of listening: sets LOW X / HIGH X around your sub and top end and trims OUTPUT so out = in. The LEVEL readout shows out vs in at all times." });
            f.entries.push_back({ "Spectrum shading", "Magenta shading from the top = that band is being pulled down; violet from the bottom = lifted. Node bells show each dynamic-EQ node's live move." });
            f.entries.push_back({ "HITS / DUCK meters", "HITS shows the transient detector (what ATTACK and SUSTAIN act on). DUCK shows the total duck from pattern plus sidechain." });
            v.push_back(f);
        }
        v.push_back(zwui::motionSection());
        v.push_back(zwui::presetSection());
        v.push_back({ "RECIPES", "",
            { { "Pump without routing", "'Pump (Pattern)' - LFO 1 Steps drives DUCK. Draw the lane: full on beat 1, falling through the bar. Change the division to 1/2 for half-time pumps." },
              { "Gate chop", "'Gate Chop' - alternating steps at 1/2 with GLIDE 0. Put it on a pad or a vocal hold." },
              { "Kick ducks the bass", "'Kick Duck (SC)' - route the kick to the sidechain input, SC AMT 80 %, SC REL to taste." },
              { "Talking resonance control", "On a growl bass, node 1 at 300 Hz RANGE -8 and node 2 wherever it whistles. Only the offending moments get cut." } } });
        return v;
    }
}
