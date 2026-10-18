// Texts and colours shared by the front end's screens.
#pragma once

#include <string>

#include "resources/lang.hpp"

namespace ab {

// Level names, original messages 150-160. A function: the names are read when shown, after the
// language has loaded.
inline std::string levelName(int level) {
    const std::string names[11] = {tr("Green Acres"),     tr("Classic Green Acres"), tr("The Hockey Rink"), tr("Ancient Egypt"),
                                   tr("The Coal Mine"),   tr("The Beach"),           tr("Aliens"),           tr("Haunted House"),
                                   tr("Under the Ocean"), tr("Deep Forest Green"),  tr("Inner City Trash")};
    return names[level >= 0 && level < 11 ? level : 0];
}

// Text colour for each player seat (the players' own colours, readable on the menu pictures).
inline constexpr float kSeatColour[10][3] = {
    {0.95f, 0.95f, 0.95f}, {0.55f, 0.55f, 0.55f}, {0.90f, 0.15f, 0.15f}, {0.20f, 0.35f, 0.95f}, {0.15f, 0.80f, 0.20f},
    {0.95f, 0.90f, 0.15f}, {0.15f, 0.85f, 0.85f}, {0.90f, 0.20f, 0.90f}, {0.95f, 0.55f, 0.10f}, {0.55f, 0.20f, 0.90f}};

}  // namespace ab
