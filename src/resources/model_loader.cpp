#include "model_loader.hpp"
#include "core/logging.hpp"

Model *ModelLoader::Load(const std::string &path) {
    return nullptr;
}

Model *ModelLoader::CreateDefault() {
    LogInfo("Default Model asset created\n");

    return new Model();
}