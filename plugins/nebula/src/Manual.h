#pragma once
#include <zenwave_ui/Manual.h>

namespace NebulaManual
{
    inline std::vector<zwui::ManualSection> sections()
    {
        using S = zwui::ManualSection;
        std::vector<S> v;
        v.push_back({ "QUICK START", "Nebula is a two-oscillator wavetable synth built around a planet: the sphere is oscillator A's table, the orbit is oscillator B, the halo is the harmonic spectrum you can carve by hand.",
            { { "1", "Pick a preset. Atmosphere = pads that sit behind the track, then Keys, Plucks, Leads, Bass." },
              { "2", "Play. The lit ring on the planet is POSITION - turn it (or let MOVEMENT do it) and the wave morphs through the table." },
              { "3", "Shape with the eight MACROS: WARMTH, AIR, MOVEMENT, SPACE, GRIT, WIDTH, ATTACK, RELEASE. Every preset is designed to be played from these." },
              { "4", "Carve: click the halo to place a band on a harmonic, drag it up to boost or down to cut. It changes the wave itself, so it costs nothing per voice." } } });

        v.push_back({ "OSCILLATORS", "A is the planet, B is the ring. Each has:",
            { { "Table", "Glass (bells, chimes), Voice (vowel formants), Metal (inharmonic clusters), Grain (dense texture), Dune (saw - square - pulse), Halo (fundamental + shimmering cluster), Ion (bright buzz with sliding combs), Void (dark sub, upper partials only at the top)." },
              { "POSITION", "Where you are in the table's 32 frames. The one knob that defines the sound; modulate it from an LFO, the step lane or the envelope for living timbre.", "aposition" },
              { "POSITION", "Oscillator B's position in its table.", "bposition" },
              { "LEVEL", "Oscillator level.", "alevel" }, { "LEVEL", "Oscillator B level (AIR raises it).", "blevel" },
              { "OCT / SEMI / FINE", "Tuning. B a fifth or an octave up with a few cents of FINE is the classic pad stack.", "aoctave" },
              { "UNISON / SPREAD / WIDTH", "Up to 8 stacked voices per oscillator; SPREAD detunes them symmetrically (centre voice stays in tune), WIDTH pans them. The WIDTH macro scales both.", "aunison" },
              { "SPREAD", "Unison detune.", "aspread" }, { "WIDTH", "Unison stereo width.", "awidth" },
              { "UNISON", "Oscillator B unison voices.", "bunison" }, { "SPREAD", "B unison detune.", "bspread" }, { "WIDTH", "B unison width.", "bwidth" },
              { "OCT", "B octave.", "boctave" }, { "SEMI", "B semitones.", "bsemis" }, { "FINE", "B fine tune in cents.", "bfine" }, { "SEMI", "A semitones.", "asemis" }, { "FINE", "A fine tune in cents.", "afine" } } });

        v.push_back({ "CARVE + FILTER", "Two ways to shape: carve the wave, then filter the voice.",
            { { "CARVE", "Up to 8 hand-placed harmonic bands on the planet's halo. Each has a centre harmonic (H1 = fundamental, H2 = octave, H3 = fifth above...), a width in octaves and a gain from fully cut to +12 dB. Click the halo to add, drag along it to move, drag up/down for gain, Shift-drag for width, double-click to remove. Carving is rendered into the wavetable, so it is sample-accurate and free at playback. Hollow H2-H4 for glassy choirs; cut the evens for a reed; boost one narrow band and it sings." },
              { "CUTOFF", "Per-voice filter cutoff. WARMTH pulls it down, AIR opens it.", "cutoff" },
              { "RES", "Filter resonance.", "res" },
              { "ENV", "How much the MOD envelope moves the cutoff (bipolar).", "envamt" },
              { "KEY", "Key tracking: 100 % = the cutoff follows the note one-to-one.", "keytrack" },
              { "DRIVE", "Per-voice saturation before the filter. The GRIT macro adds to it.", "drive" },
              { "Low / High / Band", "Filter type." } } });

        v.push_back({ "ENVELOPES + SPACE", "",
            { { "AMP A D S R", "Amplitude envelope per voice. The ATTACK and RELEASE macros scale A and R (0.1x - 10x) so presets stay playable.", "ampa" },
              { "D", "Amp decay.", "ampd" }, { "S", "Amp sustain.", "amps" }, { "R", "Amp release.", "ampr" },
              { "MOD A D S R", "Second envelope, routed to the filter through ENV (and available for anything via the Motion matrix once routed).", "moda" },
              { "D", "Mod decay.", "modd" }, { "S", "Mod sustain.", "mods" }, { "R", "Mod release.", "modr" },
              { "SPACE  SIZE / DECAY / DARK", "Umbra's reverb engine inside the synth. The SPACE macro sets how much.", "spacesize" },
              { "DECAY", "Reverb tail length.", "spacedecay" }, { "DARK", "Reverb tone.", "spacedark" },
              { "DUCK / REL", "Pads are the atmosphere, not the hero: route the vocal, lead or kick into Nebula's sidechain input and DUCK makes the synth breathe under it; REL sets how fast it comes back. Dims when nothing is connected.", "duck" },
              { "REL", "Duck release.", "duckrelease" },
              { "GLIDE", "Portamento time between notes.", "glide" },
              { "DRIFT", "Slow random pitch wander per unison voice - the analogue 'alive' feeling. MOVEMENT scales it.", "drift" },
              { "VOL", "Master volume.", "volume" } } });

        v.push_back({ "MACROS", "Eight performance knobs that reach into several places at once. Automate these, map a controller to them, or let the envelope and LFOs move them.",
            { { "WARMTH", "Lowers the filter, adds a touch of drive.", "mwarmth" },
              { "AIR", "Opens the filter, brightens the space, raises oscillator B.", "mair" },
              { "MOVEMENT", "LFO 1 into both POSITIONS, more drift, more reverb modulation.", "mmovement" },
              { "SPACE", "Reverb amount.", "mspace" },
              { "GRIT", "Per-voice saturation.", "mgrit" },
              { "WIDTH", "Unison spread and stereo width on both oscillators.", "mwidth" },
              { "ATTACK", "Scales the amp attack 0.1x - 10x.", "mattack" },
              { "RELEASE", "Scales the amp release 0.1x - 10x.", "mrelease" },
              { "Mod wheel / aftertouch", "Both push the filter open and act as pressure - expressive without any setup." } } });

        v.push_back({ "FX PAGE", "The VOICE / FX tabs switch the middle band. FX is the patch's own processing chain, in order: EQ -> SATURATE -> GLUE -> ECHO -> SPACE.",
            { { "EQ", "Four nodes on the synth's live output spectrum. Click the display to place a node, drag to move it (frequency / gain), or use the NODE buttons and FREQ / GAIN / Q knobs. Types: bell, low shelf, high shelf, low cut, high cut. Node gains are modulation targets - a slow LFO on a high shelf is a breathing pad.", "eq1freq" },
              { "GAIN", "Selected node gain.", "eq1gain" }, { "Q", "Selected node width.", "eq1q" },
              { "SATURATE", "Core-style tape stage over the whole patch. DRIVE = how hard, MIX = parallel blend.", "satdrive" }, { "MIX", "Saturation blend.", "satmix" },
              { "GLUE", "A single-band Pulse compressor: upward and downward, adaptive thresholds. DEPTH is the amount, TIME scales attack and release. Pads stay even, plucks get dense. GR shows the live gain change.", "gluedepth" }, { "TIME", "Glue timing scale.", "gluetime" },
              { "ECHO", "Synced ping-pong delay with a filtered, gently saturating feedback loop. SYNC + division or TIME in ms; FEEDBACK, MIX, TONE (dark to bright repeats).", "echofb" },
              { "TIME", "Echo time when SYNC is off.", "echotime" }, { "MIX", "Echo level.", "echomix" }, { "TONE", "Feedback filter: left = dark dub repeats, right = bright.", "echotone" } } });
        v.push_back(zwui::motionSection());
        v.push_back(zwui::presetSection());
        v.push_back({ "RECIPES", "",
            { { "Pad behind a vocal", "'Event Horizon Pad', route the vocal to Nebula's sidechain, DUCK 40 %, REL 300 ms. The pad swells into every gap." },
              { "Talking pad", "'Choir of Glass' - the step lane on POSITION A plus a boosted carve band at H9. Draw a new lane and it says something else." },
              { "Reed lead", "Dune at POSITION 0, carve bands cutting H2, H4 and H6, GRIT 30 %, GLIDE 60 ms." },
              { "Sub that stays put", "'Void Sub' - one voice, no unison, DRIFT 0. Nothing moves, by design." } } });
        return v;
    }
}
