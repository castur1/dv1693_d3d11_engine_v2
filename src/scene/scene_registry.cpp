#include "scene_registry.hpp"
#include "core/logging.hpp"

#include <filesystem>

namespace fs = std::filesystem;

void SceneRegistry::RegisterScenes() {
    LogInfo("Registering scenes...\n");

    this->sceneNames.clear();

    if (!fs::exists(this->sceneDir)) {
        LogWarn("Scenes directory '%s' does not exist\n", this->sceneDir.c_str());
        return;
    }

    for (const auto &entry : fs::directory_iterator(this->sceneDir)) {
        if (!entry.is_regular_file())
            continue;

        this->sceneNames.push_back(entry.path().stem().string());
    }

    std::sort(this->sceneNames.begin(), this->sceneNames.end());
}
