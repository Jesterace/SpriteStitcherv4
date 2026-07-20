#pragma once

#include "ss/core/color/Lab.h"

namespace ss::core::color {

// CIEDE2000 perceptual color difference. Smaller is more similar; 0 is
// identical. Chosen over CIE76/94 for accuracy on near-neutral and pastel
// tones, which are common in DMC floss ranges.
double ciede2000(const Lab& lab1, const Lab& lab2);

} // namespace ss::core::color
