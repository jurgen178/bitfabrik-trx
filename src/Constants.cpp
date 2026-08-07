#include "Constants.h"
#include "Globals.h"

// ── CONSTANT DATA DEFINITIONS ─────────────────────────────────────────────
// Defined here (without 'extern') so there is exactly one object per symbol.
// Declared extern in Globals.h for use across all translation units.

const long STEPS[] = { 10, 100, 1000, 10000 };

Band BANDS[] = {
  { "10m",  28000000, 29700000, 28500000, 0, 8,  true,  true },
  { "15m",  21000000, 21450000, 21200000, 1, 9,  true,  true },
  { "20m",  14000000, 14350000, 14200000, 2, 10, true,  true },
  { "40m",   7000000,  7300000,  7100000, 3, 11, false, true },
  { "80m",   3500000,  3800000,  3650000, 4, 12, false, true },
  { "160m",  1810000,  2000000,  1850000, 5, 13, false, true },
};
