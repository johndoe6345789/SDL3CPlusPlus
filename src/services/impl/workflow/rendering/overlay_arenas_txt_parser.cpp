#include "services/interfaces/workflow/rendering/overlay_arenas_txt_parser.hpp"

#include <zip.h>

#include <algorithm>
#include <sstream>
#include <vector>

namespace sdl3cpp::services::impl {

q3overlay::ArenaMap ParseArenasTxt(const std::string& pk3) {
    q3overlay::ArenaMap arenaData;
    int ze     = 0;
    zip_t* arc = zip_open(pk3.c_str(), ZIP_RDONLY, &ze);
    if (!arc) return arenaData;

    zip_stat_t st;
    if (zip_stat(arc, "scripts/arenas.txt", 0, &st) == 0) {
        std::vector<char> buf(st.size + 1, '\0');
        zip_file_t* zf = zip_fopen(arc, "scripts/arenas.txt", 0);
        if (zf) {
            zip_fread(zf, buf.data(), st.size);
            zip_fclose(zf);
        }
        std::string text(buf.data());
        size_t pos = 0;
        while ((pos = text.find('{', pos)) != std::string::npos) {
            size_t end = text.find('}', pos);
            if (end == std::string::npos) break;
            std::string block = text.substr(pos + 1, end - pos - 1);
            pos               = end + 1;
            std::string mapName, longName, bots;
            std::istringstream ss(block);
            std::string tok;
            while (ss >> tok) {
                auto readVal = [&]() -> std::string {
                    std::string v;
                    ss >> std::ws;
                    if (ss.peek() == '"') {
                        ss.get();
                        std::getline(ss, v, '"');
                    } else {
                        ss >> v;
                    }
                    return v;
                };
                if (tok == "map")
                    mapName = readVal();
                else if (tok == "longname")
                    longName = readVal();
                else if (tok == "bots")
                    bots = readVal();
            }
            if (!mapName.empty()) {
                std::string key = mapName;
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                std::string firstBot = bots.substr(0, bots.find(' '));
                arenaData[key]       = {longName, firstBot};
            }
        }
    }
    zip_close(arc);
    return arenaData;
}

}  // namespace sdl3cpp::services::impl
