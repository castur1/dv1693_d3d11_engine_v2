#include "asset_registry.hpp"
#include "core/logging.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static std::string TrimWhitespace(const std::string &str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    return start == std::string::npos ? "" : str.substr(start, end - start + 1);
}

void AssetRegistry::RegisterAssets() {
    LogInfo("Registering assets...\n");
    LogIndent();

    for (const auto &entry : fs::recursive_directory_iterator(this->assetDir)) {
        if (!entry.is_regular_file())
            continue;

        const fs::path &filepath = entry.path();

        if (filepath.extension() == ".meta") {
            fs::path assetPath = filepath;
            assetPath.replace_extension("");

            if (!fs::exists(assetPath)) {
                LogInfo("Deleted orphan meta file '%s'\n", filepath.generic_string().c_str());
                fs::remove(filepath);
                continue;
            }

            std::ifstream file(filepath);
            if (!file.is_open()) {
                LogWarn("Unable to open '%s'\n", filepath.generic_string().c_str());
                continue;
            }

            std::string line;
            while (std::getline(file, line)) {
                line = TrimWhitespace(line);
                
                if (line.empty() || line.front() == '#')
                    continue;

                if (line.find("uuid:") == 0) {
                    std::string uuidStr = TrimWhitespace(line.substr(5));
                    AssetID uuid = AssetID::FromString(uuidStr);

                    fs::path root = this->assetDir;
                    fs::path relative = assetPath.lexically_relative(root);
                    std::string assetPathRel = relative.generic_string().c_str();

                    this->uuidToPath[uuid] = assetPathRel;
                    this->pathToUUID[assetPathRel] = uuid;

                    LogInfo("Registered asset '%s': '%s'\n", uuidStr.c_str(), assetPathRel.c_str());
                }
            }

            file.close();
        }
        else {
            fs::path metaPath = filepath;
            metaPath += ".meta";

            if (fs::exists(metaPath))
                continue;

            LogInfo("Detected new asset '%s'\n", filepath.generic_string().c_str());

            std::ofstream file(metaPath);
            if (!file.is_open()) {
                LogWarn("Unable to open '%s'\n", metaPath.generic_string().c_str());
                continue;
            }

            AssetID uuid;
            std::string uuidStr = uuid.ToString();

            file << "uuid: " << uuidStr << std::endl;
            file.close();

            fs::path root = this->assetDir;
            fs::path relative = filepath.lexically_relative(root);

            std::string assetPathRel = relative.generic_string().c_str();

            this->uuidToPath[uuid] = assetPathRel;
            this->pathToUUID[assetPathRel] = uuid;

            LogInfo("Registered asset '%s': '%s'\n", uuidStr.c_str(), assetPathRel.c_str());
        }
    }

    LogUnindent();
}

void AssetRegistry::Clear() {
    this->uuidToPath.clear();
    this->pathToUUID.clear();
}

std::string AssetRegistry::GetPath(AssetID uuid) {
    auto iter = this->uuidToPath.find(uuid);
    if (iter == this->uuidToPath.end())
        return "";

    return iter->second;
}

AssetID AssetRegistry::GetUUID(const std::string &path) {
    auto iter = this->pathToUUID.find(path);
    if (iter == this->pathToUUID.end())
        return AssetID::invalid;

    return iter->second;
}

void AssetRegistry::SetAssetDirectory(const std::string &path) {
    this->assetDir = path;
}
