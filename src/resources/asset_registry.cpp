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

static void WriteDefaultMetadataForFileType(std::ofstream &file, const std::string &ext) {
    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".tga" || ext == ".bmp" || ext == ".gif") {
        file << "colour_space: sRGB" << std::endl;
    }
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

            std::unordered_map<std::string, std::string> data;

            std::string line;
            while (std::getline(file, line)) {
                line = TrimWhitespace(line);
                
                if (line.empty() || line.front() == '#')
                    continue;
                
                size_t colonPosition = line.find(':');
                if (colonPosition == std::string::npos)
                    continue;

                std::string key = TrimWhitespace(line.substr(0, colonPosition));
                std::string value = TrimWhitespace(line.substr(colonPosition + 1));

                if (key.empty())
                    continue;

                data[key] = value;
            }

            file.close();

            auto iter = data.find("uuid");
            if (iter == data.end()) {
                LogWarn("No uuid field in '%s'", filepath.generic_string().c_str());
                continue;
            }

            AssetID uuid = AssetID::FromString(iter->second);

            fs::path root     = this->assetDir;
            fs::path relative = assetPath.lexically_relative(root);
            std::string assetPathRel = relative.generic_string();

            this->uuidToPath[uuid] = assetPathRel;
            this->pathToUUID[assetPathRel] = uuid;

#if LOGGING_VERBOSE
            LogInfo("Registered asset '%s': '%s'\n", iter->second.c_str(), assetPathRel.c_str());
#endif

            auto &metadata = this->metadata[uuid];
            for (const auto &[key, value] : data)
                if (key != "uuid")
                    metadata[key] = value;
        }
        else {
            std::string extension = filepath.extension().generic_string();

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
            WriteDefaultMetadataForFileType(file, extension);
            file.close();

            fs::path root = this->assetDir;
            fs::path relative = filepath.lexically_relative(root);

            std::string assetPathRel = relative.generic_string().c_str();

            this->uuidToPath[uuid] = assetPathRel;
            this->pathToUUID[assetPathRel] = uuid;

#if LOGGING_VERBOSE
            LogInfo("Registered asset '%s': '%s'\n", uuidStr.c_str(), assetPathRel.c_str());
#endif
        }
    }

    LogUnindent();
}

void AssetRegistry::Clear() {
    this->uuidToPath.clear();
    this->pathToUUID.clear();
    this->metadata.clear();
}

std::string AssetRegistry::GetPath(AssetID uuid) const {
    auto iter = this->uuidToPath.find(uuid);
    if (iter == this->uuidToPath.end())
        return "";

    return iter->second;
}

AssetID AssetRegistry::GetUUID(const std::string &path) const {
    auto iter = this->pathToUUID.find(path);
    if (iter == this->pathToUUID.end())
        return AssetID::invalid;

    return iter->second;
}

std::string AssetRegistry::GetMetadata(AssetID uuid, const std::string &key, const std::string &defaultValue) const {
    auto iter = this->metadata.find(uuid);
    if (iter == this->metadata.end())
        return defaultValue;

    auto &data = iter->second;

    auto iter2 = data.find(key);
    if (iter2 == data.end())
        return defaultValue;

    return iter2->second;
}

void AssetRegistry::SetMetadata(AssetID uuid, const std::string &key, const std::string &value) {
    this->metadata[uuid][key] = value;
}

void AssetRegistry::SetAssetDirectory(const std::string &path) {
    this->assetDir = path;
}
