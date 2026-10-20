// "Sharp pixel scale": the 640x480 screen is drawn at a whole-number multiple, so that every pixel
// stays square and hard-edged, and that picture is then fitted to the window with a linear filter.
#pragma once

#include <algorithm>
#include <cmath>

namespace ab {

// The whole-number factor to draw at: the smallest one that is not below the fit of the window
// (a 1280x720 window fits 1.5, so 2; 1920x1080 fits 2.25, so 3). Never less than 1.
inline int sharpFactor(int windowW, int windowH) {
    const double fit = std::min(static_cast<double>(windowW) / 640.0, static_cast<double>(windowH) / 480.0);
    return std::clamp(static_cast<int>(std::ceil(fit - 1e-6)), 1, 4);
}

}  // namespace ab
