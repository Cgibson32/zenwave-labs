#include "Presets.h"
#include "PluginProcessor.h"
#include <zenwave_ui/Modulation.h>

namespace Presets
{
    // tables: see Tables.h (0 Glass .. 7 Void, 8..23 vocal, 24..30 atmosphere, 31..35 orchestral, 36..40 bells/keys, 41..45 analogue, 46..48 sub)
    // Osc: table, position, level, octave, semis, fine, unison, spread, width, on
    // then: cutoff, res, filterMode, envAmt, drive, drift, glide | ampA D S R | modA D S R | spaceMix size decay dark | macros warmth air movement space grit width attack release
    static const Preset kPresets[] = {
        { "Init",             "Init",       { 0, 0.3f, 0.8f, 0, 0, 0.0f, 3, 0.25f, 0.7f, true }, { 5, 0.5f, 0.5f, 0, 0, 6.0f, 2, 0.25f, 0.7f, true },
          9000.0f, 0.2f, 0, 0.2f, 0.0f, 0.25f, 0.0f, 20.0f, 300.0f, 0.8f, 600.0f, 10.0f, 400.0f, 0.3f, 400.0f, 0.25f, 0.7f, 0.6f, 0.5f, 0.3f, 0.3f, 0.2f, 0.4f, 0.0f, 0.6f, 0.5f, 0.5f },

        // --- Atmosphere (background layers that sit behind everything) ---
        { "Event Horizon Pad", "Atmosphere",{ 0, 0.55f, 0.8f, 0, 0, 0.0f, 6, 0.35f, 0.9f, true }, { 5, 0.7f, 0.6f, 1, 0, -5.0f, 4, 0.3f, 0.9f, true },
          4500.0f, 0.15f, 0, 0.3f, 0.0f, 0.4f, 0.0f, 900.0f, 1500.0f, 0.9f, 2500.0f, 2000.0f, 2000.0f, 0.5f, 2000.0f, 0.5f, 0.95f, 0.85f, 0.6f, 0.45f, 0.4f, 0.4f, 0.6f, 0.0f, 0.8f, 0.6f, 0.6f },
        { "Cold Nebula",      "Atmosphere", { 3, 0.35f, 0.6f, 0, 0, 0.0f, 5, 0.4f, 1.0f, true }, { 0, 0.2f, 0.5f, 1, 7, 4.0f, 3, 0.2f, 0.8f, true },
          3200.0f, 0.25f, 0, 0.2f, 0.0f, 0.5f, 0.0f, 1500.0f, 2000.0f, 0.85f, 3000.0f, 1000.0f, 3000.0f, 0.4f, 3000.0f, 0.55f, 1.0f, 0.9f, 0.7f, 0.5f, 0.3f, 0.5f, 0.65f, 0.0f, 0.9f, 0.65f, 0.65f },
        { "Solar Wind",       "Atmosphere", { 6, 0.3f, 0.55f, 0, 0, 0.0f, 7, 0.5f, 1.0f, true }, { 7, 0.6f, 0.7f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          2400.0f, 0.3f, 0, 0.5f, 0.1f, 0.3f, 0.0f, 600.0f, 1200.0f, 0.8f, 1800.0f, 500.0f, 2500.0f, 0.2f, 1500.0f, 0.45f, 0.9f, 0.8f, 0.5f, 0.6f, 0.2f, 0.6f, 0.55f, 0.1f, 0.85f, 0.55f, 0.55f },
        { "Choir of Glass",   "Atmosphere", { 1, 0.4f, 0.7f, 0, 0, 0.0f, 5, 0.3f, 0.9f, true }, { 0, 0.8f, 0.5f, 1, 0, 7.0f, 4, 0.25f, 0.9f, true },
          6000.0f, 0.1f, 0, 0.2f, 0.0f, 0.35f, 0.0f, 400.0f, 1000.0f, 0.9f, 1800.0f, 300.0f, 2000.0f, 0.4f, 1500.0f, 0.5f, 0.9f, 0.85f, 0.4f, 0.3f, 0.5f, 0.5f, 0.6f, 0.0f, 0.75f, 0.55f, 0.6f },

        // --- Keys ---
        { "Glass Keys",       "Keys",       { 0, 0.25f, 0.85f, 0, 0, 0.0f, 2, 0.15f, 0.5f, true }, { 5, 0.3f, 0.35f, 1, 0, 3.0f, 1, 0.0f, 0.0f, true },
          7000.0f, 0.15f, 0, 0.6f, 0.0f, 0.15f, 0.0f, 3.0f, 900.0f, 0.3f, 700.0f, 2.0f, 600.0f, 0.1f, 500.0f, 0.3f, 0.6f, 0.5f, 0.4f, 0.3f, 0.3f, 0.15f, 0.4f, 0.0f, 0.4f, 0.3f, 0.5f },
        { "Dusty Rhodes",     "Keys",       { 2, 0.15f, 0.8f, 0, 0, 0.0f, 1, 0.0f, 0.0f, true }, { 0, 0.1f, 0.5f, 0, 0, -4.0f, 2, 0.1f, 0.4f, true },
          3500.0f, 0.2f, 0, 0.7f, 0.15f, 0.2f, 0.0f, 2.0f, 1200.0f, 0.4f, 900.0f, 1.0f, 500.0f, 0.0f, 400.0f, 0.2f, 0.5f, 0.45f, 0.6f, 0.5f, 0.2f, 0.2f, 0.3f, 0.2f, 0.35f, 0.3f, 0.5f },
        { "Alien Keys",       "Keys",       { 2, 0.7f, 0.75f, 0, 0, 0.0f, 3, 0.3f, 0.7f, true }, { 6, 0.4f, 0.4f, 0, 7, 0.0f, 2, 0.2f, 0.6f, true },
          5000.0f, 0.35f, 0, 0.5f, 0.1f, 0.3f, 0.0f, 5.0f, 800.0f, 0.5f, 900.0f, 3.0f, 700.0f, 0.2f, 600.0f, 0.35f, 0.7f, 0.6f, 0.5f, 0.3f, 0.4f, 0.4f, 0.5f, 0.15f, 0.6f, 0.35f, 0.5f },

        // --- Plucks ---
        { "Halo Pluck",       "Plucks",     { 5, 0.2f, 0.85f, 0, 0, 0.0f, 3, 0.2f, 0.8f, true }, { 0, 0.5f, 0.4f, 1, 0, 5.0f, 2, 0.2f, 0.8f, true },
          2500.0f, 0.3f, 0, 0.8f, 0.0f, 0.2f, 0.0f, 1.0f, 500.0f, 0.0f, 500.0f, 1.0f, 350.0f, 0.0f, 300.0f, 0.35f, 0.6f, 0.55f, 0.4f, 0.3f, 0.4f, 0.3f, 0.5f, 0.0f, 0.6f, 0.2f, 0.4f },
        { "Ion Spark",        "Plucks",     { 6, 0.6f, 0.8f, 0, 0, 0.0f, 2, 0.15f, 0.6f, true }, { 4, 0.1f, 0.4f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          1800.0f, 0.45f, 0, 0.9f, 0.2f, 0.1f, 0.0f, 1.0f, 350.0f, 0.0f, 300.0f, 0.5f, 250.0f, 0.0f, 250.0f, 0.2f, 0.4f, 0.4f, 0.4f, 0.4f, 0.1f, 0.2f, 0.3f, 0.3f, 0.5f, 0.15f, 0.35f },

        // --- Leads ---
        { "Dune Lead",        "Leads",      { 4, 0.2f, 0.9f, 0, 0, 0.0f, 4, 0.3f, 0.5f, true }, { 4, 0.7f, 0.5f, -1, 0, -3.0f, 1, 0.0f, 0.0f, true },
          6000.0f, 0.25f, 0, 0.4f, 0.25f, 0.15f, 60.0f, 8.0f, 400.0f, 0.7f, 300.0f, 5.0f, 300.0f, 0.2f, 300.0f, 0.2f, 0.5f, 0.5f, 0.4f, 0.25f, 0.4f, 0.3f, 0.3f, 0.3f, 0.5f, 0.4f, 0.4f },
        { "Siren Call",       "Leads",      { 1, 0.6f, 0.9f, 0, 0, 0.0f, 3, 0.2f, 0.4f, true }, { 5, 0.4f, 0.35f, 1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          4000.0f, 0.4f, 0, 0.5f, 0.15f, 0.2f, 120.0f, 20.0f, 500.0f, 0.8f, 400.0f, 10.0f, 600.0f, 0.3f, 400.0f, 0.3f, 0.6f, 0.6f, 0.5f, 0.3f, 0.3f, 0.5f, 0.4f, 0.15f, 0.4f, 0.45f, 0.45f },

        // --- Bass ---
        { "Void Sub",         "Bass",       { 7, 0.2f, 1.0f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true }, { 4, 0.0f, 0.3f, 0, 0, 0.0f, 1, 0.0f, 0.0f, true },
          900.0f, 0.1f, 0, 0.5f, 0.1f, 0.0f, 40.0f, 2.0f, 300.0f, 0.9f, 200.0f, 1.0f, 300.0f, 0.0f, 200.0f, 0.0f, 0.3f, 0.3f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.3f, 0.3f },
        { "Ion Growl",        "Bass",       { 6, 0.5f, 0.9f, -1, 0, 0.0f, 2, 0.1f, 0.3f, true }, { 7, 0.3f, 0.7f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          1500.0f, 0.5f, 0, 0.7f, 0.4f, 0.1f, 50.0f, 3.0f, 400.0f, 0.8f, 250.0f, 2.0f, 500.0f, 0.3f, 250.0f, 0.1f, 0.4f, 0.4f, 0.5f, 0.5f, 0.0f, 0.3f, 0.1f, 0.5f, 0.2f, 0.3f, 0.3f },

        // =====================================================================================================
        // CINEMATIC - score-ready pads. Slow, wide, deep, with a vocal or air layer. Drop them under a scene.
        // Osc: table, position, level, octave, semis, fine, unison, spread, width, on
        // cutoff, res, mode, envAmt, drive, drift, glide | ampA D S R | modA D S R | space mix size decay dark | macros warmth air movement space grit width attack release
        { "Deep Field Organ", "Cinematic", { 44, 0.35f, 0.8f, 0, 0, 0.0f, 3, 0.2f, 0.8f, true }, { 27, 0.4f, 0.55f, -1, 0, 3.0f, 2, 0.2f, 0.7f, true },
          2860.0f, 0.1f, 0, 0.15f, 0.05f, 0.15f, 0.0f, 1200.0f, 2000.0f, 1.0f, 4000.0f, 3000.0f, 3000.0f, 0.6f, 3000.0f, 0.5f, 1.0f, 0.9f, 0.45f, 0.5f, 0.17f, 0.3f, 0.7f, 0.0f, 0.7f, 0.7f, 0.75f },
        { "Blue Moon Dawn",       "Cinematic", { 24, 0.3f, 0.8f, 0, 0, 0.0f, 6, 0.4f, 1.0f, true }, { 14, 0.3f, 0.5f, 1, 0, -6.0f, 4, 0.35f, 1.0f, true },
          3575.0f, 0.1f, 0, 0.2f, 0.0f, 0.20f, 0.0f, 1800.0f, 2500.0f, 0.95f, 5000.0f, 4000.0f, 3000.0f, 0.5f, 4000.0f, 0.55f, 0.95f, 0.9f, 0.35f, 0.4f, 0.28f, 0.5f, 0.7f, 0.0f, 0.9f, 0.75f, 0.8f },
        { "Dark Horizon",       "Cinematic", { 29, 0.4f, 0.9f, -1, 0, 0.0f, 2, 0.2f, 0.6f, true }, { 26, 0.5f, 0.5f, 0, 0, 5.0f, 5, 0.4f, 0.9f, true },
          1400.0f, 0.2f, 0, 0.4f, 0.1f, 0.20f, 0.0f, 2500.0f, 3000.0f, 1.0f, 6000.0f, 5000.0f, 3000.0f, 0.7f, 4000.0f, 0.45f, 1.0f, 0.95f, 0.75f, 0.6f, 0.10f, 0.4f, 0.6f, 0.1f, 0.7f, 0.8f, 0.85f },
        { "Frozen Cathedral",   "Cinematic", { 22, 0.35f, 0.75f, 0, 0, 0.0f, 5, 0.35f, 1.0f, true }, { 0, 0.6f, 0.45f, 1, 7, 4.0f, 3, 0.3f, 1.0f, true },
          3025.0f, 0.1f, 0, 0.2f, 0.0f, 0.20f, 0.0f, 1500.0f, 2500.0f, 0.95f, 5000.0f, 3000.0f, 3000.0f, 0.5f, 3000.0f, 0.6f, 1.0f, 0.95f, 0.4f, 0.35f, 0.30f, 0.4f, 0.75f, 0.0f, 0.85f, 0.7f, 0.8f },
        { "Slow Sunrise",       "Cinematic", { 31, 0.2f, 0.8f, 0, 0, 0.0f, 7, 0.45f, 1.0f, true }, { 33, 0.1f, 0.5f, -1, 0, -4.0f, 4, 0.3f, 0.8f, true },
          1400.0f, 0.15f, 0, 0.9f, 0.05f, 0.20f, 0.0f, 3000.0f, 4000.0f, 1.0f, 5000.0f, 6000.0f, 4000.0f, 0.9f, 4000.0f, 0.4f, 0.9f, 0.8f, 0.5f, 0.55f, 0.17f, 0.3f, 0.6f, 0.05f, 0.85f, 0.85f, 0.8f },
        { "Deep Space Drone",   "Cinematic", { 15, 0.3f, 0.9f, -1, 0, 0.0f, 3, 0.25f, 0.7f, true }, { 29, 0.6f, 0.6f, -2, 0, 0.0f, 1, 0.0f, 0.0f, true },
          1400.0f, 0.2f, 0, 0.3f, 0.1f, 0.20f, 0.0f, 2000.0f, 3000.0f, 1.0f, 5000.0f, 4000.0f, 3000.0f, 0.6f, 3000.0f, 0.4f, 1.0f, 0.95f, 0.8f, 0.7f, 0.07f, 0.5f, 0.6f, 0.05f, 0.6f, 0.75f, 0.85f },
        { "Tidal Strings",      "Cinematic", { 26, 0.3f, 0.8f, 0, 0, 0.0f, 7, 0.5f, 1.0f, true }, { 32, 0.4f, 0.55f, -1, 0, 3.0f, 3, 0.3f, 0.8f, true },
          1760.0f, 0.15f, 0, 0.5f, 0.05f, 0.20f, 0.0f, 1400.0f, 2500.0f, 0.95f, 3500.0f, 2500.0f, 3000.0f, 0.5f, 3000.0f, 0.4f, 0.9f, 0.8f, 0.45f, 0.5f, 0.20f, 0.45f, 0.6f, 0.0f, 0.8f, 0.65f, 0.7f },
        { "Event Glass",        "Cinematic", { 39, 0.3f, 0.7f, 0, 0, 0.0f, 4, 0.3f, 1.0f, true }, { 25, 0.4f, 0.6f, -1, 0, 0.0f, 3, 0.3f, 0.9f, true },
          3850.0f, 0.1f, 0, 0.2f, 0.0f, 0.20f, 0.0f, 2000.0f, 2500.0f, 1.0f, 6000.0f, 3000.0f, 3000.0f, 0.5f, 3000.0f, 0.55f, 1.0f, 0.95f, 0.3f, 0.3f, 0.33f, 0.5f, 0.75f, 0.0f, 0.9f, 0.75f, 0.85f },
        { "Breathing Planet",   "Cinematic", { 28, 0.2f, 0.8f, 0, 0, 0.0f, 5, 0.35f, 1.0f, true }, { 46, 0.3f, 0.6f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          1430.0f, 0.2f, 0, 0.6f, 0.05f, 0.20f, 0.0f, 2500.0f, 3000.0f, 1.0f, 5000.0f, 5000.0f, 3000.0f, 0.8f, 3500.0f, 0.4f, 0.95f, 0.9f, 0.55f, 0.6f, 0.15f, 0.6f, 0.6f, 0.0f, 0.75f, 0.8f, 0.8f },
        { "Last Transmission",  "Cinematic", { 30, 0.35f, 0.7f, 0, 0, 0.0f, 4, 0.4f, 1.0f, true }, { 10, 0.4f, 0.6f, 0, 0, -5.0f, 3, 0.25f, 0.8f, true },
          1650.0f, 0.25f, 0, 0.3f, 0.1f, 0.20f, 0.0f, 1500.0f, 2500.0f, 0.95f, 4500.0f, 2500.0f, 3000.0f, 0.5f, 3000.0f, 0.5f, 1.0f, 0.9f, 0.6f, 0.5f, 0.15f, 0.55f, 0.65f, 0.1f, 0.75f, 0.7f, 0.8f },

        // =====================================================================================================
        // VOCAL - choirs and voices built from real vowel formants. Meditational, soulful, human.
        // =====================================================================================================
        // HEAVENLY - real female voices (VocalSet, 9 sopranos / mezzos). Tables 49 Choir Ahh, 50 Ooh, 51 Oh, 52 Eh, 53 Ee,
        // 54-57 Soprano I-IV, 58-59 Mezzo I-II. Dreamy, ambient beds: slow, wide, always moving, soft on top.
        { "Heaven Choir",       "Heavenly",  { 49, 0.4f, 0.85f, 0, 0, 0.0f, 4, 0.14f, 1.0f, true }, { 50, 0.5f, 0.3f, 1, 0, -4.0f, 3, 0.12f, 1.0f, true },
          2600.0f, 0.08f, 0, 0.1f, 0.0f, 0.12f, 0.0f, 1800.0f, 3000.0f, 0.95f, 5000.0f, 2500.0f, 3000.0f, 0.5f, 3000.0f, 0.55f, 0.95f, 0.9f, 0.55f, 0.45f, 0.15f, 0.3f, 0.6f, 0.0f, 0.85f, 0.7f, 0.8f },
        { "Celestial Ooh",      "Heavenly",  { 50, 0.35f, 0.85f, 0, 0, 0.0f, 5, 0.16f, 1.0f, true }, { 53, 0.4f, 0.25f, 1, 0, 5.0f, 2, 0.1f, 1.0f, true },
          2200.0f, 0.1f, 0, 0.15f, 0.0f, 0.12f, 0.0f, 2500.0f, 3000.0f, 0.95f, 6000.0f, 3000.0f, 3000.0f, 0.5f, 3000.0f, 0.6f, 1.0f, 0.95f, 0.6f, 0.5f, 0.15f, 0.35f, 0.65f, 0.0f, 0.9f, 0.75f, 0.85f },
        { "Ethereal Soprano",   "Heavenly",  { 56, 0.3f, 0.8f, 0, 0, 0.0f, 3, 0.12f, 1.0f, true }, { 49, 0.6f, 0.4f, 0, 7, 3.0f, 3, 0.14f, 1.0f, true },
          3000.0f, 0.08f, 0, 0.15f, 0.0f, 0.12f, 0.0f, 1500.0f, 2500.0f, 0.95f, 4500.0f, 2500.0f, 2500.0f, 0.5f, 2500.0f, 0.55f, 1.0f, 0.9f, 0.45f, 0.4f, 0.2f, 0.35f, 0.65f, 0.0f, 0.9f, 0.7f, 0.8f },
        { "Other Planet",       "Heavenly",  { 52, 0.5f, 0.8f, 0, 0, 0.0f, 4, 0.18f, 1.0f, true }, { 39, 0.3f, 0.22f, 1, 7, 0.0f, 3, 0.2f, 1.0f, true },
          2400.0f, 0.15f, 0, 0.2f, 0.0f, 0.2f, 0.0f, 3000.0f, 4000.0f, 1.0f, 7000.0f, 4000.0f, 3000.0f, 0.6f, 3000.0f, 0.65f, 1.0f, 0.95f, 0.5f, 0.4f, 0.2f, 0.6f, 0.7f, 0.0f, 0.9f, 0.85f, 0.9f },
        { "Dream Bed",          "Heavenly",  { 51, 0.3f, 0.85f, 0, 0, 0.0f, 4, 0.12f, 1.0f, true }, { 24, 0.3f, 0.35f, -1, 0, 0.0f, 2, 0.1f, 0.8f, true },
          1900.0f, 0.1f, 0, 0.15f, 0.0f, 0.1f, 0.0f, 2500.0f, 4000.0f, 1.0f, 6000.0f, 3000.0f, 3000.0f, 0.5f, 3000.0f, 0.5f, 1.0f, 0.95f, 0.65f, 0.55f, 0.1f, 0.3f, 0.6f, 0.0f, 0.8f, 0.8f, 0.85f },
        { "Mezzo Halo",         "Heavenly",  { 58, 0.4f, 0.85f, 0, 0, 0.0f, 4, 0.14f, 1.0f, true }, { 5, 0.5f, 0.3f, 1, 0, -3.0f, 2, 0.1f, 1.0f, true },
          2400.0f, 0.1f, 0, 0.15f, 0.0f, 0.12f, 0.0f, 1600.0f, 3000.0f, 0.95f, 5000.0f, 2500.0f, 3000.0f, 0.5f, 3000.0f, 0.55f, 0.95f, 0.9f, 0.5f, 0.5f, 0.15f, 0.3f, 0.6f, 0.0f, 0.85f, 0.7f, 0.8f },
        { "Breath of Light",    "Heavenly",  { 54, 0.2f, 0.8f, 0, 0, 0.0f, 5, 0.18f, 1.0f, true }, { 25, 0.4f, 0.4f, 1, 0, -4.0f, 3, 0.2f, 1.0f, true },
          2200.0f, 0.1f, 0, 0.3f, 0.0f, 0.15f, 0.0f, 2800.0f, 3000.0f, 1.0f, 7000.0f, 4000.0f, 3000.0f, 0.6f, 3000.0f, 0.6f, 1.0f, 0.95f, 0.45f, 0.4f, 0.25f, 0.5f, 0.7f, 0.0f, 0.85f, 0.85f, 0.9f },
        { "Soul Choir",         "Heavenly",  { 59, 0.45f, 0.85f, 0, 0, 0.0f, 4, 0.16f, 1.0f, true }, { 51, 0.3f, 0.4f, -1, 0, 2.0f, 2, 0.1f, 0.7f, true },
          2400.0f, 0.12f, 0, 0.2f, 0.03f, 0.12f, 0.0f, 700.0f, 2000.0f, 0.95f, 3000.0f, 1000.0f, 1500.0f, 0.5f, 1500.0f, 0.45f, 0.9f, 0.8f, 0.6f, 0.55f, 0.15f, 0.3f, 0.5f, 0.0f, 0.75f, 0.55f, 0.7f },
        { "Vowel Drift",        "Heavenly",  { 55, 0.1f, 0.85f, 0, 0, 0.0f, 4, 0.14f, 1.0f, true }, { 57, 0.9f, 0.5f, 0, 0, -5.0f, 3, 0.14f, 1.0f, true },
          2800.0f, 0.1f, 0, 0.15f, 0.0f, 0.12f, 0.0f, 2000.0f, 3000.0f, 0.95f, 5000.0f, 3000.0f, 3000.0f, 0.5f, 3000.0f, 0.55f, 1.0f, 0.9f, 0.5f, 0.45f, 0.15f, 0.3f, 0.6f, 0.0f, 0.9f, 0.75f, 0.85f },
        { "Afterglow",          "Heavenly",  { 49, 0.6f, 0.8f, -1, 0, 0.0f, 3, 0.12f, 1.0f, true }, { 50, 0.4f, 0.5f, 0, 7, 4.0f, 3, 0.12f, 1.0f, true },
          2000.0f, 0.1f, 0, 0.2f, 0.0f, 0.12f, 0.0f, 3500.0f, 4000.0f, 1.0f, 8000.0f, 5000.0f, 3000.0f, 0.6f, 3000.0f, 0.6f, 1.0f, 0.95f, 0.65f, 0.55f, 0.1f, 0.4f, 0.6f, 0.0f, 0.85f, 0.9f, 0.95f },

        { "Ahh Choir",          "Vocal",     { 8, 0.3f, 0.85f, 0, 0, 0.0f, 5, 0.3f, 1.0f, true }, { 9, 0.4f, 0.5f, -1, 0, 4.0f, 3, 0.25f, 0.8f, true },
          2750.0f, 0.1f, 0, 0.15f, 0.0f, 0.20f, 0.0f, 600.0f, 1500.0f, 0.95f, 2500.0f, 1500.0f, 2000.0f, 0.5f, 2000.0f, 0.4f, 0.85f, 0.75f, 0.45f, 0.4f, 0.17f, 0.3f, 0.55f, 0.0f, 0.8f, 0.6f, 0.65f },
        { "Ooh Choir",          "Vocal",     { 9, 0.3f, 0.85f, 0, 0, 0.0f, 5, 0.3f, 1.0f, true }, { 19, 0.3f, 0.45f, 1, 0, -4.0f, 3, 0.25f, 0.9f, true },
          2310.0f, 0.1f, 0, 0.15f, 0.0f, 0.20f, 0.0f, 800.0f, 1500.0f, 0.95f, 3000.0f, 1500.0f, 2000.0f, 0.5f, 2000.0f, 0.45f, 0.9f, 0.8f, 0.5f, 0.45f, 0.15f, 0.3f, 0.6f, 0.0f, 0.8f, 0.65f, 0.7f },
        { "Monastery",          "Vocal",     { 11, 0.2f, 0.9f, -1, 0, 0.0f, 3, 0.2f, 0.7f, true }, { 15, 0.3f, 0.6f, -1, 7, 2.0f, 2, 0.2f, 0.6f, true },
          1650.0f, 0.15f, 0, 0.2f, 0.05f, 0.20f, 0.0f, 900.0f, 2000.0f, 1.0f, 3500.0f, 2000.0f, 2500.0f, 0.6f, 2500.0f, 0.5f, 1.0f, 0.9f, 0.6f, 0.6f, 0.10f, 0.3f, 0.6f, 0.0f, 0.6f, 0.65f, 0.75f },
        { "Soul Hum",           "Vocal",     { 10, 0.25f, 0.9f, 0, 0, 0.0f, 3, 0.2f, 0.7f, true }, { 12, 0.3f, 0.4f, 0, 0, -3.0f, 2, 0.2f, 0.6f, true },
          1540.0f, 0.15f, 0, 0.3f, 0.05f, 0.20f, 0.0f, 400.0f, 1200.0f, 0.9f, 1800.0f, 800.0f, 1500.0f, 0.5f, 1500.0f, 0.3f, 0.7f, 0.6f, 0.5f, 0.55f, 0.12f, 0.3f, 0.4f, 0.0f, 0.55f, 0.5f, 0.55f },
        { "Gospel Pad",         "Vocal",     { 18, 0.3f, 0.85f, 0, 0, 0.0f, 4, 0.3f, 0.9f, true }, { 8, 0.5f, 0.5f, 1, 0, 5.0f, 3, 0.3f, 0.9f, true },
          2475.0f, 0.1f, 0, 0.2f, 0.05f, 0.20f, 0.0f, 300.0f, 1200.0f, 0.9f, 1500.0f, 600.0f, 1500.0f, 0.4f, 1500.0f, 0.3f, 0.75f, 0.6f, 0.4f, 0.5f, 0.15f, 0.3f, 0.45f, 0.0f, 0.7f, 0.5f, 0.55f },
        { "Angel Host",         "Vocal",     { 23, 0.3f, 0.75f, 0, 0, 0.0f, 6, 0.4f, 1.0f, true }, { 14, 0.4f, 0.5f, 1, 0, -6.0f, 4, 0.35f, 1.0f, true },
          3850.0f, 0.1f, 0, 0.15f, 0.0f, 0.20f, 0.0f, 1500.0f, 2500.0f, 0.95f, 4500.0f, 3000.0f, 3000.0f, 0.5f, 3000.0f, 0.55f, 1.0f, 0.95f, 0.35f, 0.3f, 0.33f, 0.4f, 0.75f, 0.0f, 0.9f, 0.7f, 0.8f },
        { "Throat Singer",      "Vocal",     { 20, 0.1f, 0.9f, -1, 0, 0.0f, 2, 0.15f, 0.5f, true }, { 46, 0.2f, 0.5f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          1925.0f, 0.2f, 0, 0.2f, 0.1f, 0.20f, 0.0f, 500.0f, 1500.0f, 1.0f, 2500.0f, 1500.0f, 2000.0f, 0.5f, 2000.0f, 0.35f, 0.9f, 0.8f, 0.6f, 0.6f, 0.07f, 0.4f, 0.5f, 0.0f, 0.5f, 0.55f, 0.65f },
        { "Whisper Wind",       "Vocal",     { 16, 0.2f, 0.7f, 0, 0, 0.0f, 6, 0.5f, 1.0f, true }, { 25, 0.5f, 0.5f, 0, 0, 0.0f, 3, 0.3f, 1.0f, true },
          4400.0f, 0.15f, 0, 0.2f, 0.0f, 0.20f, 0.0f, 2000.0f, 3000.0f, 0.95f, 5000.0f, 3000.0f, 3000.0f, 0.5f, 3000.0f, 0.5f, 1.0f, 0.9f, 0.3f, 0.3f, 0.35f, 0.6f, 0.7f, 0.0f, 0.9f, 0.75f, 0.8f },
        { "Lullaby Voice",      "Vocal",     { 19, 0.2f, 0.9f, 0, 0, 0.0f, 2, 0.15f, 0.6f, true }, { 34, 0.2f, 0.4f, 1, 0, 2.0f, 1, 0.0f, 0.0f, true },
          1925.0f, 0.1f, 0, 0.3f, 0.0f, 0.20f, 0.0f, 120.0f, 800.0f, 0.85f, 1200.0f, 200.0f, 800.0f, 0.3f, 800.0f, 0.35f, 0.8f, 0.7f, 0.5f, 0.5f, 0.15f, 0.2f, 0.5f, 0.0f, 0.5f, 0.35f, 0.5f },
        { "Soul Lead",          "Vocal",     { 12, 0.4f, 0.9f, 0, 0, 0.0f, 2, 0.15f, 0.4f, true }, { 17, 0.3f, 0.4f, 0, 0, -3.0f, 1, 0.0f, 0.0f, true },
          2475.0f, 0.2f, 0, 0.4f, 0.1f, 0.20f, 90.0f, 25.0f, 600.0f, 0.8f, 500.0f, 15.0f, 600.0f, 0.3f, 400.0f, 0.3f, 0.7f, 0.5f, 0.4f, 0.5f, 0.15f, 0.3f, 0.4f, 0.1f, 0.4f, 0.3f, 0.45f },

        // =====================================================================================================
        // ATMOSPHERE II / KEYS / PLUCKS / BASS with the new tables
        { "Aurora Field",       "Atmosphere",{ 24, 0.2f, 0.8f, 0, 0, 0.0f, 6, 0.4f, 1.0f, true }, { 39, 0.2f, 0.4f, 1, 0, 5.0f, 3, 0.3f, 1.0f, true },
          6000.0f, 0.1f, 0, 0.2f, 0.0f, 0.3f, 0.0f, 1500.0f, 2500.0f, 0.95f, 4000.0f, 3000.0f, 3000.0f, 0.5f, 3000.0f, 0.5f, 0.95f, 0.9f, 0.4f, 0.4f, 0.55f, 0.5f, 0.7f, 0.0f, 0.85f, 0.7f, 0.75f },
        { "Nimbus Cloud",       "Atmosphere",{ 25, 0.4f, 0.8f, 0, 0, 0.0f, 5, 0.35f, 1.0f, true }, { 28, 0.3f, 0.5f, -1, 0, -3.0f, 3, 0.3f, 0.9f, true },
          4000.0f, 0.15f, 0, 0.3f, 0.0f, 0.35f, 0.0f, 2000.0f, 3000.0f, 0.95f, 4500.0f, 3500.0f, 3000.0f, 0.5f, 3000.0f, 0.45f, 0.95f, 0.85f, 0.5f, 0.45f, 0.4f, 0.5f, 0.65f, 0.0f, 0.8f, 0.75f, 0.75f },
        { "Celesta Dream",      "Keys",      { 36, 0.3f, 0.85f, 0, 0, 0.0f, 2, 0.15f, 0.6f, true }, { 39, 0.1f, 0.3f, 1, 0, 3.0f, 2, 0.2f, 0.8f, true },
          9000.0f, 0.1f, 0, 0.5f, 0.0f, 0.1f, 0.0f, 2.0f, 1500.0f, 0.2f, 1800.0f, 1.0f, 800.0f, 0.1f, 600.0f, 0.4f, 0.85f, 0.75f, 0.35f, 0.3f, 0.5f, 0.3f, 0.6f, 0.0f, 0.6f, 0.3f, 0.6f },
        { "Music Box Memory",   "Keys",      { 37, 0.3f, 0.85f, 1, 0, 0.0f, 1, 0.0f, 0.0f, true }, { 36, 0.2f, 0.3f, 1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          8000.0f, 0.1f, 0, 0.6f, 0.0f, 0.1f, 0.0f, 1.0f, 900.0f, 0.1f, 1200.0f, 1.0f, 500.0f, 0.0f, 400.0f, 0.35f, 0.8f, 0.7f, 0.3f, 0.3f, 0.45f, 0.2f, 0.5f, 0.0f, 0.5f, 0.3f, 0.5f },
        { "Tine Piano",         "Keys",      { 40, 0.2f, 0.9f, 0, 0, 0.0f, 1, 0.0f, 0.0f, true }, { 46, 0.1f, 0.35f, 0, 0, 0.0f, 1, 0.0f, 0.0f, true },
          4000.0f, 0.15f, 0, 0.7f, 0.1f, 0.1f, 0.0f, 2.0f, 1400.0f, 0.3f, 900.0f, 1.0f, 450.0f, 0.0f, 400.0f, 0.2f, 0.5f, 0.4f, 0.5f, 0.55f, 0.3f, 0.2f, 0.3f, 0.1f, 0.4f, 0.3f, 0.45f },
        { "Kalimba Rain",       "Plucks",    { 38, 0.3f, 0.9f, 1, 0, 0.0f, 1, 0.0f, 0.0f, true }, { 39, 0.1f, 0.25f, 2, 0, 0.0f, 1, 0.0f, 0.0f, true },
          6000.0f, 0.2f, 0, 0.7f, 0.0f, 0.05f, 0.0f, 1.0f, 600.0f, 0.0f, 700.0f, 1.0f, 300.0f, 0.0f, 300.0f, 0.3f, 0.7f, 0.65f, 0.3f, 0.35f, 0.5f, 0.3f, 0.45f, 0.0f, 0.5f, 0.3f, 0.45f },
        { "FM Bell Garden",     "Plucks",    { 45, 0.35f, 0.85f, 0, 0, 0.0f, 2, 0.15f, 0.7f, true }, { 0, 0.3f, 0.3f, 1, 0, 4.0f, 2, 0.2f, 0.8f, true },
          9000.0f, 0.1f, 0, 0.6f, 0.0f, 0.1f, 0.0f, 1.0f, 1200.0f, 0.1f, 1500.0f, 1.0f, 500.0f, 0.0f, 400.0f, 0.4f, 0.85f, 0.75f, 0.3f, 0.3f, 0.55f, 0.3f, 0.6f, 0.0f, 0.6f, 0.3f, 0.55f },
        { "Super Saw Sky",      "Leads",     { 41, 0.4f, 0.85f, 0, 0, 0.0f, 7, 0.5f, 1.0f, true }, { 41, 0.6f, 0.6f, 1, 0, -7.0f, 5, 0.4f, 1.0f, true },
          8000.0f, 0.1f, 0, 0.3f, 0.1f, 0.2f, 0.0f, 20.0f, 600.0f, 0.9f, 900.0f, 10.0f, 400.0f, 0.3f, 400.0f, 0.3f, 0.8f, 0.6f, 0.3f, 0.4f, 0.5f, 0.3f, 0.5f, 0.1f, 0.9f, 0.3f, 0.5f },
        { "Reese Floor",        "Bass",      { 47, 0.3f, 0.9f, -1, 0, 0.0f, 2, 0.1f, 0.4f, true }, { 46, 0.1f, 0.6f, -2, 0, 0.0f, 1, 0.0f, 0.0f, true },
          1200.0f, 0.3f, 0, 0.5f, 0.3f, 0.0f, 30.0f, 3.0f, 400.0f, 0.9f, 250.0f, 2.0f, 400.0f, 0.2f, 250.0f, 0.05f, 0.3f, 0.3f, 0.6f, 0.6f, 0.1f, 0.2f, 0.1f, 0.3f, 0.3f, 0.3f, 0.4f },
        { "808 Sub",            "Bass",      { 48, 0.2f, 1.0f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true }, { 46, 0.0f, 0.4f, -2, 0, 0.0f, 1, 0.0f, 0.0f, false },
          700.0f, 0.05f, 0, 0.4f, 0.15f, 0.0f, 60.0f, 1.0f, 900.0f, 0.6f, 300.0f, 1.0f, 500.0f, 0.0f, 200.0f, 0.0f, 0.3f, 0.3f, 0.6f, 0.6f, 0.0f, 0.1f, 0.0f, 0.2f, 0.2f, 0.2f, 0.4f },
    };

    int count() { return int(std::size(kPresets)); }
    const Preset& get(int index) { return kPresets[juce::jlimit(0, count() - 1, index)]; }
    juce::String name(int index) { return get(index).name; }
    juce::String category(int index) { return get(index).category; }

    static void setParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float plainValue)
    {
        if (auto* p = apvts.getParameter(id)) { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(plainValue)); p->endChangeGesture(); }
    }

    void apply(int index, juce::AudioProcessorValueTreeState& apvts)
    {
        const auto& p = get(index);
        const Osc* oscs[2] = { &p.a, &p.b };
        for (int o = 0; o < 2; ++o)
        {
            setParam(apvts, ParamIDs::osc(o, "table"), float(oscs[o]->table)); setParam(apvts, ParamIDs::osc(o, "position"), oscs[o]->position);
            setParam(apvts, ParamIDs::osc(o, "level"), oscs[o]->level); setParam(apvts, ParamIDs::osc(o, "octave"), float(oscs[o]->octave));
            setParam(apvts, ParamIDs::osc(o, "semis"), float(oscs[o]->semis)); setParam(apvts, ParamIDs::osc(o, "fine"), oscs[o]->fine);
            setParam(apvts, ParamIDs::osc(o, "unison"), float(oscs[o]->unison)); setParam(apvts, ParamIDs::osc(o, "spread"), oscs[o]->spread);
            setParam(apvts, ParamIDs::osc(o, "width"), oscs[o]->width); setParam(apvts, ParamIDs::osc(o, "on"), oscs[o]->on ? 1.0f : 0.0f);
        }
        setParam(apvts, ParamIDs::cutoff, p.cutoff); setParam(apvts, ParamIDs::res, p.res); setParam(apvts, ParamIDs::filterMode, float(p.filterMode));
        setParam(apvts, ParamIDs::envAmt, p.envAmt); setParam(apvts, ParamIDs::keyTrack, 0.5f); setParam(apvts, ParamIDs::drive, p.drive);
        setParam(apvts, ParamIDs::drift, p.drift); setParam(apvts, ParamIDs::glide, p.glideMs);
        setParam(apvts, ParamIDs::ampA, p.ampA); setParam(apvts, ParamIDs::ampD, p.ampD); setParam(apvts, ParamIDs::ampS, p.ampS); setParam(apvts, ParamIDs::ampR, p.ampR);
        setParam(apvts, ParamIDs::modA, p.modA); setParam(apvts, ParamIDs::modD, p.modD); setParam(apvts, ParamIDs::modS, p.modS); setParam(apvts, ParamIDs::modR, p.modR);
        setParam(apvts, ParamIDs::spaceMix, p.spaceMix); setParam(apvts, ParamIDs::spaceSize, p.spaceSize); setParam(apvts, ParamIDs::spaceDecay, p.spaceDecay); setParam(apvts, ParamIDs::spaceDark, p.spaceDark);
        setParam(apvts, ParamIDs::mWarmth, p.warmth); setParam(apvts, ParamIDs::mAir, p.air); setParam(apvts, ParamIDs::mMovement, p.movement); setParam(apvts, ParamIDs::mSpace, p.space);
        setParam(apvts, ParamIDs::mGrit, p.grit); setParam(apvts, ParamIDs::mWidth, p.width); setParam(apvts, ParamIDs::mAttack, p.attack); setParam(apvts, ParamIDs::mRelease, p.release);
        setParam(apvts, ParamIDs::carveOn, 1.0f);
        for (int i = 0; i < nebula::kCarveBands; ++i) setParam(apvts, ParamIDs::carveBandOn(i), 0.0f);
        setParam(apvts, ParamIDs::duck, 0.0f); setParam(apvts, ParamIDs::duckRelease, 250.0f);
        setParam(apvts, ParamIDs::vibrato, 0.0f); setParam(apvts, ParamIDs::breath, 0.0f); setParam(apvts, ParamIDs::evolve, 0.0f);
        setParam(apvts, ParamIDs::chorusOn, 0.0f); setParam(apvts, ParamIDs::chorusMode, 2.0f); setParam(apvts, ParamIDs::chorusRate, 0.4f); setParam(apvts, ParamIDs::chorusDepth, 0.5f); setParam(apvts, ParamIDs::chorusMix, 0.5f); setParam(apvts, ParamIDs::chorusFb, 0.0f);
        // FX page defaults
        const float eqDef[4] = { 120.0f, 500.0f, 2500.0f, 9000.0f }; const int eqTypeDef[4] = { 1, 0, 0, 2 };
        for (int i = 0; i < 4; ++i) { setParam(apvts, ParamIDs::eqOn(i), 0.0f); setParam(apvts, ParamIDs::eqType(i), float(eqTypeDef[i])); setParam(apvts, ParamIDs::eqFreq(i), eqDef[i]); setParam(apvts, ParamIDs::eqGain(i), 0.0f); setParam(apvts, ParamIDs::eqQ(i), 1.0f); }
        setParam(apvts, ParamIDs::satOn, 0.0f); setParam(apvts, ParamIDs::satType, 0.0f); setParam(apvts, ParamIDs::satDrive, 0.3f); setParam(apvts, ParamIDs::satTone, 0.0f); setParam(apvts, ParamIDs::satBias, 0.0f); setParam(apvts, ParamIDs::satLow, 20.0f); setParam(apvts, ParamIDs::satOut, 0.0f); setParam(apvts, ParamIDs::satMix, 1.0f);
        setParam(apvts, ParamIDs::glueOn, 0.0f); setParam(apvts, ParamIDs::glueDepth, 0.5f); setParam(apvts, ParamIDs::glueTime, 0.5f); setParam(apvts, ParamIDs::glueUp, 1.0f); setParam(apvts, ParamIDs::glueDown, 1.0f); setParam(apvts, ParamIDs::glueMix, 1.0f); setParam(apvts, ParamIDs::glueOut, 0.0f);
        setParam(apvts, ParamIDs::echoWobble, 0.0f); setParam(apvts, ParamIDs::echoWidth, 0.66f);
        setParam(apvts, ParamIDs::spaceOn, 1.0f); setParam(apvts, ParamIDs::spacePre, 0.0f); setParam(apvts, ParamIDs::spaceLow, 90.0f); setParam(apvts, ParamIDs::spaceShimmer, 0.0f);
        setParam(apvts, ParamIDs::echoOn, 0.0f); setParam(apvts, ParamIDs::echoSync, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoTime, 375.0f);
        setParam(apvts, ParamIDs::echoFb, 0.4f); setParam(apvts, ParamIDs::echoMix, 0.25f); setParam(apvts, ParamIDs::echoTone, 0.5f); setParam(apvts, ParamIDs::echoPP, 1.0f);
        setParam(apvts, ParamIDs::power, 1.0f);

        // carve + motion per preset (targets 1-based: PosA=1 PosB Cutoff Res Drive LevelA LevelB Space Warmth Air Movement Grit Width Drift Duck=15)
        const juce::String n(p.name);
        auto band = [&](int i, float centre, float widthOct, float gain) { setParam(apvts, ParamIDs::carveBandOn(i), 1.0f); setParam(apvts, ParamIDs::carveBandCentre(i), centre); setParam(apvts, ParamIDs::carveBandWidth(i), widthOct); setParam(apvts, ParamIDs::carveBandGain(i), gain); };
        if (n == "Choir of Glass")    { band(0, 2.0f, 0.4f, 0.2f); band(1, 3.0f, 0.3f, 0.3f); band(2, 9.0f, 0.5f, 2.0f);
                                        zwui::setLfo(apvts, 0, 0, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.20f); zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.25f); }
        if (n == "Event Horizon Pad") { zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.30f); zwui::setLfo(apvts, 1, 6, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 3, 0.15f); }
        if (n == "Cold Nebula")       { band(0, 1.0f, 0.3f, 1.0f); band(1, 6.0f, 1.0f, 0.3f); zwui::setLfo(apvts, 0, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 2, 0.35f); }
        if (n == "Solar Wind")        { band(0, 4.0f, 0.8f, 0.15f); band(1, 24.0f, 0.6f, 2.5f); zwui::setLfo(apvts, 0, 1, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 3, 0.30f); zwui::setModSlot(apvts, 1, 1, 13, 0.20f); }
        if (n == "Alien Keys")        { band(0, 2.0f, 0.3f, 0.0f); band(1, 7.0f, 0.4f, 3.0f); band(2, 11.0f, 0.3f, 2.5f); }
        if (n == "Siren Call")        { band(0, 3.0f, 0.5f, 2.5f); band(1, 5.0f, 0.4f, 0.3f); zwui::setLfo(apvts, 0, 0, false, 8, 5.5f); zwui::setModSlot(apvts, 0, 1, 1, 0.08f); }
        if (n == "Ion Growl")         { band(0, 3.0f, 0.4f, 3.0f); band(1, 8.0f, 0.6f, 0.2f); zwui::setLfo(apvts, 0, 7, true, 10, 1.0f); zwui::setModSlot(apvts, 0, 1, 3, 0.40f); zwui::setModSlot(apvts, 1, 1, 1, 0.30f); }
        // ---- cinematic
        if (n == "Deep Field Organ") { zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.25f); zwui::setLfo(apvts, 1, 6, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 10, 0.2f);
                                        setParam(apvts, ParamIDs::spacePre, 30.0f); setParam(apvts, ParamIDs::spaceLow, 120.0f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.3f); setParam(apvts, ParamIDs::glueTime, 0.9f); setParam(apvts, ParamIDs::glueUp, 0.6f);
                                        setParam(apvts, ParamIDs::eqOn(0), 1.0f); setParam(apvts, ParamIDs::eqGain(0), -2.5f); }
        if (n == "Blue Moon Dawn")       { band(0, 3.0f, 0.5f, 1.8f); band(1, 7.0f, 0.6f, 0.4f); zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.35f); zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.3f); zwui::setModSlot(apvts, 2, 2, 13, 0.15f);
                                        setParam(apvts, ParamIDs::spaceShimmer, 1.0f); setParam(apvts, ParamIDs::spacePre, 50.0f); setParam(apvts, ParamIDs::eqOn(3), 1.0f); setParam(apvts, ParamIDs::eqGain(3), 2.0f); }
        if (n == "Dark Horizon")       { band(0, 1.0f, 0.4f, 1.3f); band(1, 5.0f, 0.8f, 0.3f); zwui::setLfo(apvts, 0, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 3, 0.3f); zwui::setLfo(apvts, 1, 0, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.25f);
                                        setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 0.0f); setParam(apvts, ParamIDs::satDrive, 0.25f); setParam(apvts, ParamIDs::satLow, 100.0f); setParam(apvts, ParamIDs::satMix, 0.5f);
                                        setParam(apvts, ParamIDs::spaceLow, 150.0f); setParam(apvts, ParamIDs::spacePre, 20.0f); }
        if (n == "Frozen Cathedral")   { band(0, 2.0f, 0.3f, 0.5f); band(1, 6.0f, 0.5f, 1.6f); zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.3f); zwui::setLfo(apvts, 1, 6, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 24, 0.12f);
                                        setParam(apvts, ParamIDs::spaceShimmer, 1.0f); setParam(apvts, ParamIDs::spacePre, 60.0f); setParam(apvts, ParamIDs::spaceLow, 140.0f); setParam(apvts, ParamIDs::eqOn(3), 1.0f); setParam(apvts, ParamIDs::eqGain(3), 1.5f); }
        if (n == "Slow Sunrise")       { zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 2, 0.4f); zwui::setModSlot(apvts, 1, 3, 3, 0.5f);
                                        setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.35f); setParam(apvts, ParamIDs::glueTime, 1.0f); setParam(apvts, ParamIDs::spacePre, 40.0f); setParam(apvts, ParamIDs::spaceLow, 110.0f); }
        if (n == "Deep Space Drone")   { band(0, 1.0f, 0.3f, 1.4f); band(1, 3.0f, 0.4f, 1.3f); band(2, 8.0f, 1.0f, 0.3f); zwui::setLfo(apvts, 0, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.3f); zwui::setLfo(apvts, 1, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 3, 0.2f);
                                        setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 1.0f); setParam(apvts, ParamIDs::satDrive, 0.3f); setParam(apvts, ParamIDs::satBias, 0.3f); setParam(apvts, ParamIDs::satLow, 80.0f); setParam(apvts, ParamIDs::satMix, 0.6f); setParam(apvts, ParamIDs::spaceLow, 160.0f); }
        if (n == "Tidal Strings")      { zwui::setLfo(apvts, 0, 0, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.35f); zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 3, 0.2f); zwui::setModSlot(apvts, 2, 2, 7, 0.2f);
                                        setParam(apvts, ParamIDs::spacePre, 35.0f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.25f); setParam(apvts, ParamIDs::glueTime, 0.8f); }
        if (n == "Event Glass")        { band(0, 4.0f, 0.4f, 2.0f); band(1, 9.0f, 0.4f, 1.8f); zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.3f); zwui::setLfo(apvts, 1, 6, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.3f);
                                        setParam(apvts, ParamIDs::spaceShimmer, 1.0f); setParam(apvts, ParamIDs::spacePre, 70.0f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 9.0f); setParam(apvts, ParamIDs::echoFb, 0.5f); setParam(apvts, ParamIDs::echoMix, 0.15f); setParam(apvts, ParamIDs::echoTone, 0.65f); }
        if (n == "Breathing Planet")   { zwui::setLfo(apvts, 0, 0, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.45f); zwui::setModSlot(apvts, 1, 1, 3, 0.25f); zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 2, 2, 27, 0.2f);
                                        setParam(apvts, ParamIDs::spacePre, 25.0f); setParam(apvts, ParamIDs::spaceLow, 130.0f); }
        if (n == "Last Transmission")  { band(0, 2.0f, 0.3f, 0.3f); band(1, 5.0f, 0.5f, 2.0f); zwui::setLfo(apvts, 0, 7, true, 8, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.25f); zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 3, 0.3f);
                                        setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.55f); setParam(apvts, ParamIDs::echoMix, 0.3f); setParam(apvts, ParamIDs::echoTone, 0.3f); setParam(apvts, ParamIDs::echoWobble, 0.35f);
                                        setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 0.0f); setParam(apvts, ParamIDs::satDrive, 0.3f); setParam(apvts, ParamIDs::satTone, 0.3f); setParam(apvts, ParamIDs::satMix, 0.6f); }
        // ---- vocal: human motion on every voice preset
        auto human = [&](float vib, float br) { setParam(apvts, ParamIDs::vibrato, vib); setParam(apvts, ParamIDs::breath, br); };
        if (juce::String(p.category) == "Vocal" || n == "Frozen Cathedral" || n == "Blue Moon Dawn" || n == "Last Transmission" || n == "Choir of Glass") human(0.25f, 0.2f);
        // ---- heavenly recipe: ensemble chorus, big dark space with pre-delay, high shelf tamed, slow per-voice motion
        auto heavenly = [&](float vib, float br, float ev, bool shimmer, float hiShelfDb, float chorusMix)
        {
            human(vib, br); setParam(apvts, ParamIDs::evolve, ev);
            setParam(apvts, ParamIDs::chorusOn, 1.0f); setParam(apvts, ParamIDs::chorusMode, 2.0f); setParam(apvts, ParamIDs::chorusRate, 0.3f); setParam(apvts, ParamIDs::chorusDepth, 0.5f); setParam(apvts, ParamIDs::chorusMix, chorusMix);
            setParam(apvts, ParamIDs::spacePre, 60.0f); setParam(apvts, ParamIDs::spaceLow, 170.0f); setParam(apvts, ParamIDs::spaceShimmer, shimmer ? 1.0f : 0.0f);
            setParam(apvts, ParamIDs::eqOn(3), 1.0f); setParam(apvts, ParamIDs::eqType(3), 2.0f); setParam(apvts, ParamIDs::eqFreq(3), 5500.0f); setParam(apvts, ParamIDs::eqGain(3), hiShelfDb);
            setParam(apvts, ParamIDs::eqOn(0), 1.0f); setParam(apvts, ParamIDs::eqType(0), 1.0f); setParam(apvts, ParamIDs::eqFreq(0), 160.0f); setParam(apvts, ParamIDs::eqGain(0), -2.0f);
            setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.2f); setParam(apvts, ParamIDs::glueTime, 1.0f); setParam(apvts, ParamIDs::glueUp, 0.5f);
            zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.25f);
            zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 3, 0.15f); zwui::setModSlot(apvts, 2, 2, 2, 0.2f);
        };
        if (juce::String(p.category) == "Heavenly") heavenly(0.25f, 0.25f, 0.65f, true, -3.0f, 0.45f);
        if (n == "Heaven Choir")       { setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 9.0f); setParam(apvts, ParamIDs::echoFb, 0.35f); setParam(apvts, ParamIDs::echoMix, 0.08f); setParam(apvts, ParamIDs::echoTone, 0.25f); }
        if (n == "Celestial Ooh")      { heavenly(0.2f, 0.3f, 0.7f, true, -4.0f, 0.5f); setParam(apvts, ParamIDs::spacePre, 80.0f); }
        if (n == "Ethereal Soprano")   { heavenly(0.35f, 0.3f, 0.5f, true, -2.5f, 0.35f); }
        if (n == "Other Planet")       { heavenly(0.15f, 0.2f, 0.9f, true, -3.0f, 0.55f); setParam(apvts, ParamIDs::chorusMode, 4.0f); setParam(apvts, ParamIDs::chorusRate, 0.15f); setParam(apvts, ParamIDs::chorusDepth, 0.6f); setParam(apvts, ParamIDs::chorusFb, 0.35f); setParam(apvts, ParamIDs::chorusMix, 0.4f); band(0, 7.0f, 0.5f, 1.8f); zwui::setModSlot(apvts, 3, 2, 27, 0.2f);
                                        setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 11.0f); setParam(apvts, ParamIDs::echoFb, 0.5f); setParam(apvts, ParamIDs::echoMix, 0.12f); setParam(apvts, ParamIDs::echoTone, 0.3f); setParam(apvts, ParamIDs::echoWobble, 0.3f); }
        if (n == "Dream Bed")          { heavenly(0.15f, 0.2f, 0.75f, false, -4.0f, 0.5f); setParam(apvts, ParamIDs::spaceLow, 140.0f); }
        if (n == "Mezzo Halo")         { heavenly(0.3f, 0.25f, 0.6f, true, -3.0f, 0.4f); }
        if (n == "Breath of Light")    { heavenly(0.2f, 0.5f, 0.8f, true, -2.0f, 0.5f); setParam(apvts, ParamIDs::spacePre, 90.0f); setParam(apvts, ParamIDs::spaceLow, 200.0f); }
        if (n == "Soul Choir")         { heavenly(0.3f, 0.2f, 0.4f, false, -3.0f, 0.3f); setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 1.0f); setParam(apvts, ParamIDs::satDrive, 0.15f); setParam(apvts, ParamIDs::satBias, 0.2f); setParam(apvts, ParamIDs::satMix, 0.35f); }
        if (n == "Vowel Drift")        { heavenly(0.25f, 0.25f, 0.6f, true, -3.0f, 0.45f); zwui::setLfo(apvts, 0, 1, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.45f); zwui::setModSlot(apvts, 2, 1, 2, -0.45f); }
        if (n == "Afterglow")          { heavenly(0.2f, 0.3f, 0.7f, true, -4.0f, 0.5f); setParam(apvts, ParamIDs::chorusMode, 3.0f); setParam(apvts, ParamIDs::chorusRate, 0.1f); setParam(apvts, ParamIDs::chorusDepth, 0.5f); setParam(apvts, ParamIDs::chorusFb, 0.3f); setParam(apvts, ParamIDs::chorusMix, 0.3f); setParam(apvts, ParamIDs::spacePre, 100.0f); setParam(apvts, ParamIDs::spaceDecay, 0.98f); }
        if (n == "Ahh Choir")          { zwui::setLfo(apvts, 0, 0, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.3f); zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.25f); setParam(apvts, ParamIDs::spacePre, 40.0f); setParam(apvts, ParamIDs::spaceLow, 150.0f);
                                        setParam(apvts, ParamIDs::eqOn(1), 1.0f); setParam(apvts, ParamIDs::eqFreq(1), 320.0f); setParam(apvts, ParamIDs::eqGain(1), -2.0f); }
        if (n == "Ooh Choir")          { zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.35f); zwui::setLfo(apvts, 1, 6, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 7, 0.2f); setParam(apvts, ParamIDs::spacePre, 50.0f); setParam(apvts, ParamIDs::spaceLow, 140.0f); }
        if (n == "Monastery")          { band(0, 2.0f, 0.3f, 1.3f); zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.5f); zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.2f);
                                        setParam(apvts, ParamIDs::spacePre, 80.0f); setParam(apvts, ParamIDs::spaceLow, 120.0f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.3f); setParam(apvts, ParamIDs::glueTime, 0.9f); }
        if (n == "Soul Hum")           { zwui::setLfo(apvts, 0, 0, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.4f); zwui::setModSlot(apvts, 1, 3, 3, 0.3f); setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 1.0f); setParam(apvts, ParamIDs::satDrive, 0.2f); setParam(apvts, ParamIDs::satBias, 0.25f); setParam(apvts, ParamIDs::satMix, 0.5f); }
        if (n == "Gospel Pad")         { zwui::setLfo(apvts, 0, 0, false, 10, 5.2f); zwui::setModSlot(apvts, 0, 1, 1, 0.06f); zwui::setLfo(apvts, 1, 0, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.3f);
                                        setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.3f); setParam(apvts, ParamIDs::echoMix, 0.12f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.3f); }
        if (n == "Angel Host")         { band(0, 6.0f, 0.5f, 1.6f); zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.3f); zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.3f);
                                        setParam(apvts, ParamIDs::spaceShimmer, 1.0f); setParam(apvts, ParamIDs::spacePre, 60.0f); setParam(apvts, ParamIDs::spaceLow, 180.0f); }
        if (n == "Throat Singer")      { zwui::setLfo(apvts, 0, 1, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.5f); zwui::setLfo(apvts, 1, 6, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 3, 0.15f); setParam(apvts, ParamIDs::spaceLow, 160.0f); }
        if (n == "Whisper Wind")       { zwui::setLfo(apvts, 0, 6, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.5f); zwui::setLfo(apvts, 1, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 3, 0.35f); zwui::setModSlot(apvts, 2, 2, 13, 0.2f); setParam(apvts, ParamIDs::spacePre, 40.0f); setParam(apvts, ParamIDs::spaceLow, 250.0f); }
        if (n == "Lullaby Voice")      { zwui::setLfo(apvts, 0, 0, false, 10, 4.8f); zwui::setModSlot(apvts, 0, 1, 1, 0.05f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 8.0f); setParam(apvts, ParamIDs::echoFb, 0.35f); setParam(apvts, ParamIDs::echoMix, 0.18f); setParam(apvts, ParamIDs::echoTone, 0.35f); setParam(apvts, ParamIDs::spacePre, 30.0f); }
        if (n == "Soul Lead")          { zwui::setLfo(apvts, 0, 0, false, 10, 5.5f); zwui::setModSlot(apvts, 0, 1, 1, 0.08f); zwui::setEnv(apvts, 5.0f, 300.0f, 0.3f, 300.0f); zwui::setModSlot(apvts, 1, 3, 3, 0.4f);
                                        setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 1.0f); setParam(apvts, ParamIDs::satDrive, 0.35f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.35f); setParam(apvts, ParamIDs::echoMix, 0.2f); }
        // ---- atmosphere II / keys / plucks / bass
        if (n == "Aurora Field")       { zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.5f); zwui::setLfo(apvts, 1, 6, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.3f); setParam(apvts, ParamIDs::spaceShimmer, 1.0f); setParam(apvts, ParamIDs::spacePre, 45.0f); }
        if (n == "Nimbus Cloud")       { zwui::setLfo(apvts, 0, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.4f); zwui::setLfo(apvts, 1, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 3, 0.3f); setParam(apvts, ParamIDs::spacePre, 30.0f); setParam(apvts, ParamIDs::spaceLow, 120.0f); }
        if (n == "Celesta Dream")      { setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.4f); setParam(apvts, ParamIDs::echoMix, 0.22f); setParam(apvts, ParamIDs::echoTone, 0.6f); setParam(apvts, ParamIDs::spacePre, 20.0f); }
        if (n == "Music Box Memory")   { setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 9.0f); setParam(apvts, ParamIDs::echoFb, 0.45f); setParam(apvts, ParamIDs::echoMix, 0.25f); setParam(apvts, ParamIDs::echoTone, 0.4f); setParam(apvts, ParamIDs::echoWobble, 0.5f); setParam(apvts, ParamIDs::spaceShimmer, 1.0f); }
        if (n == "Tine Piano")         { setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 1.0f); setParam(apvts, ParamIDs::satDrive, 0.3f); setParam(apvts, ParamIDs::satBias, 0.35f); zwui::setLfo(apvts, 0, 0, false, 10, 0.8f); zwui::setModSlot(apvts, 0, 1, 7, 0.15f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.35f); setParam(apvts, ParamIDs::glueTime, 0.3f); }
        if (n == "Kalimba Rain")       { setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.5f); setParam(apvts, ParamIDs::echoMix, 0.3f); setParam(apvts, ParamIDs::echoTone, 0.55f); setParam(apvts, ParamIDs::echoWidth, 1.0f); }
        if (n == "FM Bell Garden")     { zwui::setEnv(apvts, 1.0f, 400.0f, 0.0f, 300.0f); zwui::setModSlot(apvts, 0, 3, 1, 0.5f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.4f); setParam(apvts, ParamIDs::echoMix, 0.2f); setParam(apvts, ParamIDs::spaceShimmer, 1.0f); }
        if (n == "Super Saw Sky")      { zwui::setLfo(apvts, 0, 0, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.3f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.4f); setParam(apvts, ParamIDs::eqOn(0), 1.0f); setParam(apvts, ParamIDs::eqGain(0), -4.0f); setParam(apvts, ParamIDs::spacePre, 20.0f); }
        if (n == "Reese Floor")        { setParam(apvts, ParamIDs::spaceOn, 0.0f); zwui::setLfo(apvts, 0, 0, true, 10, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.4f); setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 0.0f); setParam(apvts, ParamIDs::satDrive, 0.4f); setParam(apvts, ParamIDs::satLow, 110.0f);
                                        setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.5f); setParam(apvts, ParamIDs::glueUp, 0.3f); setParam(apvts, ParamIDs::glueTime, 0.4f); }
        if (n == "808 Sub")            { setParam(apvts, ParamIDs::spaceOn, 0.0f); setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 0.0f); setParam(apvts, ParamIDs::satDrive, 0.25f); setParam(apvts, ParamIDs::satMix, 0.4f); zwui::setEnv(apvts, 1.0f, 120.0f, 0.0f, 100.0f); zwui::setModSlot(apvts, 0, 3, 1, 0.4f); }
        if (n == "Dune Lead")         { zwui::setEnv(apvts, 2.0f, 200.0f, 0.3f, 200.0f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.35f); setParam(apvts, ParamIDs::echoMix, 0.2f); setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satDrive, 0.35f); }
        if (n == "Choir of Glass")    { setParam(apvts, ParamIDs::spaceShimmer, 1.0f); setParam(apvts, ParamIDs::spacePre, 40.0f); }
        if (n == "Cold Nebula")       { setParam(apvts, ParamIDs::spacePre, 60.0f); setParam(apvts, ParamIDs::spaceLow, 160.0f); }
        if (n == "Dusty Rhodes")      { setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 1.0f); setParam(apvts, ParamIDs::satDrive, 0.3f); setParam(apvts, ParamIDs::satTone, -0.25f); setParam(apvts, ParamIDs::satBias, 0.3f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoWobble, 0.45f); setParam(apvts, ParamIDs::echoMix, 0.15f); setParam(apvts, ParamIDs::echoTone, 0.3f); }
        if (n == "Siren Call")        { setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 5.0f); setParam(apvts, ParamIDs::satDrive, 0.4f); setParam(apvts, ParamIDs::satLow, 150.0f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 6.0f); setParam(apvts, ParamIDs::echoFb, 0.45f); setParam(apvts, ParamIDs::echoMix, 0.22f); setParam(apvts, ParamIDs::echoWidth, 1.0f); }
        if (n == "Void Sub")          { setParam(apvts, ParamIDs::spaceOn, 0.0f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.35f); setParam(apvts, ParamIDs::glueUp, 0.3f); setParam(apvts, ParamIDs::glueTime, 0.8f); }
        if (n == "Glass Keys")        { setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 4.0f); setParam(apvts, ParamIDs::echoFb, 0.3f); setParam(apvts, ParamIDs::echoMix, 0.18f); setParam(apvts, ParamIDs::eqOn(0), 1.0f); setParam(apvts, ParamIDs::eqGain(0), -3.0f); }
        if (n == "Event Horizon Pad") { setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.4f); setParam(apvts, ParamIDs::glueTime, 0.7f); setParam(apvts, ParamIDs::eqOn(0), 1.0f); setParam(apvts, ParamIDs::eqGain(0), -4.0f); setParam(apvts, ParamIDs::eqOn(3), 1.0f); setParam(apvts, ParamIDs::eqGain(3), 2.0f); }
        if (n == "Ion Growl")         { setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 2.0f); setParam(apvts, ParamIDs::satLow, 120.0f); setParam(apvts, ParamIDs::satDrive, 0.55f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.6f); setParam(apvts, ParamIDs::glueTime, 0.3f); setParam(apvts, ParamIDs::eqOn(1), 1.0f); setParam(apvts, ParamIDs::eqFreq(1), 350.0f); setParam(apvts, ParamIDs::eqGain(1), -3.0f); }
        if (n == "Halo Pluck")        { setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.45f); setParam(apvts, ParamIDs::echoMix, 0.3f); setParam(apvts, ParamIDs::echoTone, 0.4f); }
        if (n == "Halo Pluck")        { band(0, 2.0f, 0.3f, 1.8f); band(1, 4.0f, 0.3f, 1.5f); }
    }
}
