#include "Constants.h"
#include "Globals.h"

// ── CONSTANT DATA DEFINITIONS ─────────────────────────────────────────────
// Defined here (without 'extern') so there is exactly one object per symbol.
// Declared extern in Globals.h for use across all translation units.

const long STEPS[] = { 10, 100, 1000, 10000 };

const Band BANDS[] = {
  { "10m",  10, 28000000, 29700000, 28500000, 0, 8,  true,  true },
  { "15m",  15, 21000000, 21450000, 21200000, 1, 9,  true,  true },
  { "20m",  20, 14000000, 14350000, 14200000, 2, 10, true,  true },
  { "40m",  40,  7000000,  7300000,  7100000, 3, 11, false, true },
  { "80m",  80,  3500000,  3800000,  3650000, 4, 12, false, true },
  { "160m", 160, 1810000,  2000000,  1850000, 5, 13, false, true },
};

const Preset PRESETS[6][3] = {
  { {"FT8", 28074000}, {"CW", 28500000}, {"FM", 29600000} },
  { {"FT8", 21074000}, {"CW", 21150000}, {"SSB", 21300000} },
  { {"FT8", 14074000}, {"CW", 14100000}, {"SSB", 14250000} },
  { {"CW", 7030000}, {"FT8", 7074000}, {"SSB", 7150000} },
  { {"CW", 3500000}, {"FT8", 3573000}, {"SSB", 3650000} },
  { {"CW", 1810000}, {"FT8", 1840000}, {"SSB", 1850000} },
};
