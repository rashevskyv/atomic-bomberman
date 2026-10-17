// Files the player puts in a "mods" folder over the game's own data: graphics, sounds, levels,
// anything the game reads from its data folder, in the same layout (data/schemes/NAME.sch,
// and so on). Copied over the data folder when the game starts.
#pragma once

#include <string>

namespace ab {

// Copies every file under `mods` to the same relative path under `target`, unless the file
// there has the same size and is not older. Makes `mods` (with a note in it) if it is not there.
// Returns how many files were copied.
int applyMods(const std::string& mods, const std::string& target);

}  // namespace ab
