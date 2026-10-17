#include "app/mods.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

namespace ab {

namespace {
constexpr const char* kNote = "README.txt";
}

int applyMods(const std::string& mods, const std::string& target) {
    std::error_code ec;
    fs::create_directories(mods, ec);
    if (!fs::exists(fs::path(mods) / kNote, ec)) {
        std::ofstream note(fs::path(mods) / kNote);
        note << "Atomic Bomberman (modern): files of your own\n\n"
                "Put files here in the same layout as the game's data folder (for instance\n"
                "data/schemes/MY.SCH for a level of your own, or a replacement picture or sound\n"
                "under the same name as the one it replaces). At every start they are copied\n"
                "over the data the game plays with.\n\n"
                "They stay there once copied: to get the original data back, delete the folder\n"
                "\"assets\" next to this one (it is made again from your copy of the game).\n";
    }
    int copied = 0;
    for (auto it = fs::recursive_directory_iterator(mods, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        const fs::path relative = fs::relative(it->path(), mods, ec);
        if (ec || relative == kNote) continue;
        const fs::path to = fs::path(target) / relative;
        std::error_code e2;
        if (fs::exists(to, e2) && fs::file_size(to, e2) == it->file_size(e2) && fs::last_write_time(to, e2) >= it->last_write_time(e2)) continue;
        fs::create_directories(to.parent_path(), e2);
        if (fs::copy_file(it->path(), to, fs::copy_options::overwrite_existing, e2)) {
            ++copied;
            std::fprintf(stderr, "INFO  Mod file %s\n", relative.string().c_str());
        } else {
            std::fprintf(stderr, "WARN  Could not copy the mod file %s: %s\n", relative.string().c_str(), e2.message().c_str());
        }
    }
    return copied;
}

}  // namespace ab
