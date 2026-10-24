#include "resources/asset_import.hpp"

#include "resources/unused_voices.hpp"

#include "resources/mve_file.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <thread>
#include <vector>

namespace ab {

namespace fs = std::filesystem;

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Entry of `dir` whose name equals `name` ignoring case; empty path if none.
fs::path findEntry(const fs::path& dir, const std::string& name) {
    std::error_code ec;
    const std::string want = lower(name);
    for (const auto& e : fs::directory_iterator(dir, ec))
        if (lower(e.path().filename().string()) == want) return e.path();
    return {};
}

fs::path findPath(fs::path dir, std::initializer_list<const char*> parts) {
    for (const char* p : parts) {
        dir = findEntry(dir, p);
        if (dir.empty()) return {};
    }
    return dir;
}

bool copyFile(const fs::path& from, const fs::path& to, ImportReport& report, std::error_code& why) {
    std::error_code ec;
    fs::create_directories(to.parent_path(), ec);
    // Files copied from a CD arrive read-only, and a read-only file cannot be written over:
    // without this a second import copied nothing (and said "1 data files").
    if (fs::exists(to, ec)) {
        fs::permissions(to, fs::perms::owner_write, fs::perm_options::add, ec);
        fs::remove(to, ec);
    }
    ec.clear();
    // Streams, not fs::copy_file: on the Switch's C library copy_file fails with "Function not implemented".
    std::ifstream in(from, std::ios::binary);
    std::ofstream out(to, std::ios::binary);
    if (in && out && in.peek() != std::char_traits<char>::eof()) out << in.rdbuf();  // an empty file adds nothing, and `<<` would set failbit
    out.flush();
    if (!in || !out) {
        why = std::error_code(errno, std::generic_category());
        return false;
    }
    std::error_code ignored;
    fs::permissions(to, fs::perms::owner_write, fs::perm_options::add, ignored);
    ++report.dataFiles;
    report.bytes += static_cast<long long>(fs::file_size(to, ec));
    return true;
}

void put32(std::vector<char>& v, std::uint32_t x) {
    for (int k = 0; k < 4; ++k) v.push_back(static_cast<char>((x >> (8 * k)) & 0xFF));
}
void put16(std::vector<char>& v, std::uint16_t x) {
    v.push_back(static_cast<char>(x & 0xFF));
    v.push_back(static_cast<char>(x >> 8));
}

// .rss is raw PCM: 22050 Hz, stereo, 16-bit signed little-endian. A .wav is the same data behind a header.
bool convertRssToWav(const fs::path& from, const fs::path& to, ImportReport& report) {
    std::ifstream in(from, std::ios::binary);
    if (!in) return false;
    const std::vector<char> pcm((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<char> h;
    const auto n = static_cast<std::uint32_t>(pcm.size());
    h.insert(h.end(), {'R', 'I', 'F', 'F'});
    put32(h, 36 + n);
    h.insert(h.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    put32(h, 16);
    put16(h, 1);          // PCM
    put16(h, 2);          // channels
    put32(h, 22050);      // sample rate
    put32(h, 22050 * 4);  // bytes per second
    put16(h, 4);          // block align
    put16(h, 16);         // bits per sample
    h.insert(h.end(), {'d', 'a', 't', 'a'});
    put32(h, n);
    std::error_code ec;
    fs::create_directories(to.parent_path(), ec);
    std::ofstream out(to, std::ios::binary);
    if (!out) return false;
    out.write(h.data(), static_cast<std::streamsize>(h.size()));
    out.write(pcm.data(), static_cast<std::streamsize>(pcm.size()));
    out.flush();
    if (!out) return false;
    ++report.sounds;
    report.bytes += static_cast<long long>(h.size() + pcm.size());
    return static_cast<bool>(out);
}

// A write to a slow or busy memory card can fail once and work on the next try: three tries,
// and the reason of the last failure goes to the log (a console has no other way to tell).
template <class Try>
bool withRetries(const fs::path& to, Try attempt) {
    std::error_code why;
    for (int tryNo = 0; tryNo < 3; ++tryNo) {
        if (tryNo > 0) std::this_thread::sleep_for(std::chrono::milliseconds(200));
        errno = 0;
        why.clear();
        if (attempt(why)) return true;
        if (!why) why = std::error_code(errno, std::generic_category());
    }
    std::fprintf(stderr, "WARN  import: cannot write %s: %s (%d)\n", to.string().c_str(), why.message().c_str(), why.value());
    return false;
}

}  // namespace

bool looksLikeOriginalGame(const std::string& folder) {
    const fs::path src(folder);
    const fs::path res = findPath(src, {"data", "res"});
    return !findEntry(src, "color.pal").empty() && !res.empty() && !findEntry(res, "valuelst.res").empty();
}

std::string findOriginalGame(const std::string& folder) {
    if (folder.empty()) return {};
    if (looksLikeOriginalGame(folder)) return folder;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(folder, ec))
        if (e.is_directory(ec) && looksLikeOriginalGame(e.path().string())) return e.path().string();
    return {};
}

ImportReport importAssets(const std::string& source, const std::string& destination, bool allSounds,
                          const std::function<bool(const ImportProgress&)>& progress) {
    ImportReport report;
    const fs::path src(source);
    const fs::path dst(destination);
    const fs::path palette = findEntry(src, "color.pal");
    const fs::path res = findPath(src, {"data", "res"});
    if (palette.empty() || res.empty() || findEntry(res, "valuelst.res").empty()) {
        report.error = "not an Atomic Bomberman folder (color.pal and data/res/valuelst.res are required): " + source;
        return report;
    }
    std::error_code ec;
    fs::create_directories(dst, ec);
    if (ec) {
        report.error = "cannot create " + destination;
        return report;
    }

    // First the list of everything there is to do, so that progress can be told; then the work.
    struct Copy {
        fs::path from, to;
    };
    std::vector<Copy> copies;
    auto collect = [&](const fs::path& dir, const fs::path& to, std::initializer_list<const char*> extensions) {
        if (dir.empty()) return;
        std::error_code scan;
        for (const auto& e : fs::directory_iterator(dir, scan)) {
            if (!e.is_regular_file(scan)) continue;
            const std::string ext = lower(e.path().extension().string());
            if (std::find_if(extensions.begin(), extensions.end(), [&](const char* x) { return ext == x; }) == extensions.end()) continue;
            copies.push_back({e.path(), to / lower(e.path().filename().string())});
        }
    };
    collect(src, dst, {".pal", ".rmp", ".fon", ".bm"});                       // .bm: help pages
    collect(res, dst / "data" / "res", {".pcx", ".res", ".cam"});             // .cam: campaigns
    collect(findPath(src, {"data", "schemes"}), dst / "data" / "schemes", {".sch"});
    collect(findPath(src, {"data", "ani"}), dst / "data" / "ani", {".ani", ".ali"});

    // Sounds named in soundlst.res ("id,name" lines; ';' starts a comment), the unused
    // voice clips that have a place (unused_voices.hpp), and with `allSounds` the other
    // sound files of the folder too.
    struct Sound {
        std::string name;
        fs::path rss;   // empty: listed but not on the disc
        bool extra;
    };
    std::vector<Sound> sounds;
    const fs::path soundDir = findPath(src, {"data", "sound"});
    std::vector<std::string> done;
    {
        // One look at the folder instead of one per sound (a slow memory card makes the difference).
        std::vector<std::pair<std::string, fs::path>> onDisc;
        std::error_code scan;
        if (!soundDir.empty())
            for (const auto& e : fs::directory_iterator(soundDir, scan))
                if (e.is_regular_file(scan) && lower(e.path().extension().string()) == ".rss") onDisc.emplace_back(lower(e.path().stem().string()), e.path());
        std::sort(onDisc.begin(), onDisc.end());
        auto find = [&](const std::string& name) -> fs::path {
            const auto it = std::lower_bound(onDisc.begin(), onDisc.end(), std::make_pair(name, fs::path()));
            return it != onDisc.end() && it->first == name ? it->second : fs::path();
        };
        std::ifstream list(findEntry(res, "soundlst.res"), std::ios::binary);
        std::string line;
        while (!soundDir.empty() && std::getline(list, line)) {
            if (auto semi = line.find(';'); semi != std::string::npos) line.erase(semi);
            const auto comma = line.find(',');
            if (comma == std::string::npos) continue;
            std::string name = line.substr(comma + 1);
            name.erase(std::remove_if(name.begin(), name.end(), [](unsigned char c) { return std::isspace(c) != 0; }), name.end());
            name = lower(name);
            if (name.empty() || std::find(done.begin(), done.end(), name) != done.end()) continue;
            done.push_back(name);
            sounds.push_back({name, find(name), false});
        }
        // The voice clips the game never plays but this implementation can (the option
        // "Unused voice clips") always come along.
        for (const auto& [name, path] : onDisc)
            if ((allSounds || unusedVoiceSeries(name) != 0) && std::find(done.begin(), done.end(), name) == done.end())
                sounds.push_back({name, path, true});
    }

    ImportProgress state;
    state.total = static_cast<int>(copies.size() + sounds.size()) + 1;
    state.dataFiles = static_cast<int>(copies.size()) + 1;
    // False: the caller wants the work stopped.
    const auto startedAt = std::chrono::steady_clock::now();
    bool loggedSounds = false;
    auto tell = [&](const std::string& item, bool isSound) {
        state.item = item;
        state.sounds = isSound;
        // A line in the log every 100 files (a console has no other way to tell how far it got).
        if (state.done % 100 == 0 || (isSound && !loggedSounds)) {
            loggedSounds = loggedSounds || isSound;
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt).count();
            std::fprintf(stderr, "INFO  import: %s %d of %d (%s) after %lld ms\n", isSound ? "sounds" : "data", state.done, state.total, item.c_str(),
                         static_cast<long long>(ms));
        }
        if (progress && !progress(state)) {
            report.cancelled = true;
            report.error = "stopped before the end";
            return false;
        }
        ++state.done;
        return true;
    };

    for (const Copy& c : copies) {
        if (!tell(c.to.filename().string(), false)) return report;
        // Done by an earlier, interrupted run: the same size is the same file (a cut-short copy is not).
        std::error_code eTo, eFrom;
        const bool there = fs::exists(c.to, eTo) && !eTo;
        if (there && fs::file_size(c.to, eTo) == fs::file_size(c.from, eFrom) && !eTo && !eFrom) {
            ++report.dataFiles;
            continue;
        }
        if (!withRetries(c.to, [&](std::error_code& why) { return copyFile(c.from, c.to, report, why); })) ++report.failed;
    }

    // The intro movie sits at the end of the original's small player program; only the
    // movie is taken, not the program.
    if (!tell("intro.mve", false)) return report;
    if (const fs::path player = findEntry(findEntry(src, "intro"), "bmintro.exe"); !player.empty()) {
        MveDecoder movie;
        if (movie.open(player.string())) {
            const auto& all = movie.fileBytes();
            std::ofstream out(dst / "intro.mve", std::ios::binary);
            out.write(reinterpret_cast<const char*>(all.data() + movie.movieOffset()),
                      static_cast<std::streamsize>(all.size() - movie.movieOffset()));
            out.flush();
            if (!out) ++report.failed;
            if (out) {
                ++report.dataFiles;
                report.bytes += static_cast<long long>(all.size() - movie.movieOffset());
            }
        }
    }

    for (const Sound& snd : sounds) {
        if (!tell(snd.name + ".wav", true)) return report;
        const int before = report.sounds;
        const fs::path wav = dst / "data" / "sound" / (snd.name + ".wav");
        std::error_code wavErr;
        const bool made = fs::exists(wav, wavErr) && fs::file_size(wav, wavErr) > 0 && !wavErr;  // an earlier run made it
        const bool ok = made || (!snd.rss.empty() && withRetries(wav, [&](std::error_code&) { return convertRssToWav(snd.rss, wav, report); }));
        if (made && !snd.rss.empty()) ++report.sounds;
        if (snd.extra) {
            report.sounds = before;  // counted apart
            report.extraSounds += ok ? 1 : 0;
        } else if (snd.rss.empty()) {
            ++report.missingSounds;
            report.missing.push_back(snd.name);
        }
        if (!ok && !snd.rss.empty()) ++report.failed;
    }
    state.item.clear();
    if (progress) progress(state);
    if (report.failed > 0) {
        report.error = std::to_string(report.failed) + " files could not be written (is the storage full?)";
        return report;
    }

    std::ofstream note(dst / "ABOUT-THESE-FILES.txt");
    note << "These files were produced on this computer from the owner's copy of\n"
            "Atomic Bomberman (c) 1997 Interplay Productions / Hudson Soft, by the\n"
            "modern reimplementation's --import-assets command.\n"
            "They are copyrighted material of their owners. Do not redistribute them.\n";
    report.ok = true;
    return report;
}

namespace {
const char* const kStampFile = "converted-by.txt";

// What the stamp of a converted folder says: its release, whether the sounds the game never
// plays were asked for, and whether the conversion was finished (a stamp written at the start
// of a conversion says it is not yet, so that an interrupted one can go on).
struct Stamp {
    std::string release;
    bool allSounds = false;
    bool complete = false;
};

Stamp readStamp(const fs::path& dir) {
    Stamp s;
    std::ifstream in(dir / kStampFile);
    std::string line;
    bool first = true;
    while (std::getline(in, line)) {
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back())) != 0) line.pop_back();
        if (first) {
            s.release = line;
            first = false;
        } else if (line == "all-sounds") {
            s.allSounds = true;
        } else if (line == "complete") {
            s.complete = true;
        }
    }
    return s;
}
}  // namespace

std::string importStamp(const std::string& converted) {
    return readStamp(converted).release;
}

bool importStampAllSounds(const std::string& converted) {
    return readStamp(converted).allSounds;
}

bool writeImportStamp(const std::string& converted, const std::string& release, bool allSounds, bool complete) {
    std::ofstream stamp(fs::path(converted) / kStampFile);
    stamp << release << "\n";
    if (allSounds) stamp << "all-sounds\n";
    if (complete) stamp << "complete\n";
    stamp.flush();
    return static_cast<bool>(stamp);
}

bool importIsDue(const std::string& converted, const std::string& release) {
    return importStamp(converted) != release;
}

ImportReport importForRelease(const std::string& source, const std::string& converted, const std::string& release,
                              const std::function<bool(const ImportProgress&)>& progress, bool allSounds) {
    const fs::path target(converted);
    fs::path work = target;
    work += ".new";
    std::error_code ec;
    // The work folder is kept when a conversion stops or fails, so that the next start goes on from
    // the files it has. It is started anew only when it was made for another release or other sounds.
    const Stamp left = readStamp(work);
    const bool sameConversion = left.release == release && left.allSounds == allSounds;
    std::fprintf(stderr, "INFO  import: release %s from %s into %s (%s)\n", release.c_str(), source.c_str(), work.string().c_str(),
                 !sameConversion ? "from the start" : left.complete ? "already converted" : "going on from the files there");
    ImportReport report;
    report.ok = true;
    if (!(sameConversion && left.complete)) {
        if (!sameConversion) fs::remove_all(work, ec);
        ec.clear();
        fs::create_directories(work, ec);
        if (!writeImportStamp(work.string(), release, allSounds, false)) {
            report.ok = false;
            report.error = "cannot write into " + work.string();
            return report;
        }
        report = importAssets(source, work.string(), allSounds, progress);
        if (!report.ok) return report;
        if (!writeImportStamp(work.string(), release, allSounds, true)) {
            report.ok = false;
            report.error = "cannot write into " + work.string();
            return report;
        }
    }
    // Put the finished folder in place. The old one goes out of the way first; what the
    // file system does not let go of is moved aside instead (the Switch reported "File exists").
    std::fprintf(stderr, "INFO  import: converted, putting %s in place of %s\n", work.string().c_str(), target.string().c_str());
    fs::remove_all(target, ec);
    if (ec) std::fprintf(stderr, "WARN  import: removing the old folder: %s (%d)\n", ec.message().c_str(), ec.value());
    ec.clear();
    if (fs::exists(target, ec)) {
        std::fprintf(stderr, "WARN  import: the old folder is still there, moving it aside\n");
        fs::path old = target;
        old += ".old";
        fs::remove_all(old, ec);
        ec.clear();
        fs::rename(target, old, ec);
        ec.clear();
    }
    fs::rename(work, target, ec);
    if (ec) {
        // The finished folder stays in the work place: the next start puts it in place.
        report.ok = false;
        report.error = "cannot put the converted files in place: " + ec.message();
        std::fprintf(stderr, "ERROR import: rename failed: %s (%d)\n", ec.message().c_str(), ec.value());
        return report;
    }
    std::fprintf(stderr, "INFO  import: done\n");
    fs::remove_all(target.string() + ".old", ec);
    return report;
}

}  // namespace ab
