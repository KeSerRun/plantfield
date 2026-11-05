#include <cstdint>
#include <string>
#include <vector>

#include "data/gardeners.h"
#include "data/templates.h"
#include "game/match.h"
#include "platform/console.h"
#include "ui/viewer.h"

int main(int argc, char** argv) {
    setupConsoleUtf8();

    std::uint32_t seed = 12345;
    bool verboseCombat = false;
    bool consoleMode = false;
    int autoRounds = 0;
    int gardener = 0;
    std::string shotPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-c" || arg == "--console") {
            consoleMode = true;
        } else if (arg == "-v") {
            verboseCombat = true;
        } else if (arg == "-g" && i + 1 < argc) {
            try {
                gardener = std::stoi(argv[++i]);
            } catch (...) {
            }
        } else if (arg == "-shot" && i + 1 < argc) {
            shotPath = argv[++i];
        } else if (arg == "-a" && i + 1 < argc) {
            try {
                autoRounds = std::stoi(argv[++i]);
            } catch (...) {
            }
        } else {
            try {
                seed = static_cast<std::uint32_t>(std::stoul(arg));
            } catch (...) {
            }
        }
    }

    const std::vector<PlantTemplate> pool = makePlantPool();
    const std::vector<Gardener> gardeners = makeGardenerPool();

    if (consoleMode) {
        Match match(pool, gardeners, seed, gardener, verboseCombat);
        match.run();
        return 0;
    }

    detachConsole();
    Viewer viewer(pool, gardeners, seed, gardener, autoRounds);
    if (!shotPath.empty()) {
        viewer.setScreenshot(shotPath);
    }
    viewer.run();
    return 0;
}
