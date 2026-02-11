#include "scene_loader.hpp"
#include "scene/component_registry.hpp"
#include "core/logging.hpp"
#include "scene/scene.hpp"
#include "resources/asset_manager.hpp"
#include "scene/component.hpp"
#include "scene/entity.hpp"

#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

class FieldWriter : public ComponentRegistry::Inspector {
    std::unordered_map<std::string, std::string> fieldValues;

    XMFLOAT2 ParseVector2(std::string str) {
        XMFLOAT2 result{};

        str.erase(std::remove(str.begin(), str.end(), '['), str.end());
        str.erase(std::remove(str.begin(), str.end(), ']'), str.end());
        str.erase(std::remove(str.begin(), str.end(), ' '), str.end());

        std::istringstream stream(str);
        std::string token;

        if (std::getline(stream, token, ',')) 
            result.x = std::stof(token);
        if (std::getline(stream, token, ',')) 
            result.y = std::stof(token);

        return result;
    }

    XMFLOAT3 ParseVector3(std::string str) {
        XMFLOAT3 result{};

        str.erase(std::remove(str.begin(), str.end(), '['), str.end());
        str.erase(std::remove(str.begin(), str.end(), ']'), str.end());
        str.erase(std::remove(str.begin(), str.end(), ' '), str.end());

        std::istringstream stream(str);
        std::string token;

        if (std::getline(stream, token, ',')) 
            result.x = std::stof(token);
        if (std::getline(stream, token, ',')) 
            result.y = std::stof(token);
        if (std::getline(stream, token, ',')) 
            result.z = std::stof(token);

        return result;
    }

    XMFLOAT4 ParseVector4(std::string str) {
        XMFLOAT4 result{};

        str.erase(std::remove(str.begin(), str.end(), '['), str.end());
        str.erase(std::remove(str.begin(), str.end(), ']'), str.end());
        str.erase(std::remove(str.begin(), str.end(), ' '), str.end());

        std::istringstream stream(str);
        std::string token;

        if (std::getline(stream, token, ',')) 
            result.x = std::stof(token);
        if (std::getline(stream, token, ',')) 
            result.y = std::stof(token);
        if (std::getline(stream, token, ',')) 
            result.z = std::stof(token);
        if (std::getline(stream, token, ',')) 
            result.w = std::stof(token);

        return result;
    }

public:
    void SetFieldValue(const std::string &name, const std::string &value) {
        this->fieldValues[name] = value;
    }

    void Field(const std::string &name, int &val) override {
        auto iter = this->fieldValues.find(name);
        if (iter != this->fieldValues.end())
            val = std::stoi(iter->second);
    }

    void Field(const std::string &name, unsigned int &val) override {
        auto iter = this->fieldValues.find(name);
        if (iter != this->fieldValues.end())
            val = static_cast<unsigned int>(std::stoul(iter->second));
    }

    void Field(const std::string &name, float &val) override {
        auto iter = this->fieldValues.find(name);
        if (iter != this->fieldValues.end())
            val = std::stof(iter->second);
    }

    void Field(const std::string &name, bool &val) override {
        auto iter = this->fieldValues.find(name);
        if (iter != this->fieldValues.end())
            val = (iter->second == "true" || iter->second == "True" || iter->second == "1");
    }

    void Field(const std::string &name, std::string &val) override {
        auto iter = this->fieldValues.find(name);
        if (iter != this->fieldValues.end())
            val = iter->second;
    }

    void Field(const std::string &name, XMFLOAT2 &val) override {
        auto iter = this->fieldValues.find(name);
        if (iter != this->fieldValues.end())
            val = FieldWriter::ParseVector2(iter->second);
    }

    void Field(const std::string &name, XMFLOAT3 &val) override {
        auto iter = this->fieldValues.find(name);
        if (iter != this->fieldValues.end())
            val = FieldWriter::ParseVector3(iter->second);
    }

    void Field(const std::string &name, XMFLOAT4 &val) override {
        auto iter = this->fieldValues.find(name);
        if (iter != this->fieldValues.end())
            val = FieldWriter::ParseVector4(iter->second);
    }

    void Field(const std::string &name, UUID_ &val) override {
        auto iter = this->fieldValues.find(name);
        if (iter != this->fieldValues.end())
            val = UUID_::FromString(iter->second);
    }
};

static int GetIndentLevel(const std::string &line) {
    int spaces = 0;

    while (line[spaces] == ' ')
        ++spaces;

    return spaces;
}

static std::string TrimWhitespace(const std::string &str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    return start == std::string::npos ? "" : str.substr(start, end - start + 1);
}

static std::pair<std::string, std::string> ParseKeyValuePair(const std::string &line) {
    size_t colonPosition = line.find(':');
    if (colonPosition == std::string::npos)
        return {"", ""};

    std::string key = TrimWhitespace(line.substr(0, colonPosition));
    std::string value = TrimWhitespace(line.substr(colonPosition + 1));

    if (value.length() >= 2 && value.front() == '"' && value.back() == '"')
        value = value.substr(1, value.length() - 2);

    return {key, value};
}

bool SceneLoader::Load(Scene &scene, const std::string &name) {
    LogInfo("\n");
    LogInfo("Loading scene '%s'...\n", name.c_str());
    LogIndent();

    std::string path = this->sceneDir + name + ".txt";

    std::ifstream file(path);
    if (!file.is_open()) {
        LogError("Failed to open file '%s'", path.c_str());
        LogUnindent();
        return false;
    }

    scene.Clear();

    if (this->assetManager)
        this->assetManager->MarkAllAssetsUnused();

    std::vector<AssetID> assets;

    Entity *currentEntity = nullptr;
    Component *currentComponent = nullptr;
    std::string currentComponentType;

    FieldWriter fieldWriter;

    std::string line;
    int lineNumber = 0;
    bool isInAssetSection = false;

    while (std::getline(file, line)) {
        ++lineNumber;

        int indent = GetIndentLevel(line);

        line = TrimWhitespace(line);
        if (line.empty() || line.front() == '#')
            continue;

        if (indent == 0) {
            if (currentComponent && !currentComponentType.empty()) {
                currentComponent->Reflect(&fieldWriter);

                currentComponent = nullptr;
                currentComponentType.clear();
            }

            if (line.find("entity ") == 0) {
                if (isInAssetSection) {
                    isInAssetSection = false;
                    LogUnindent();
                }

                if (currentEntity)
                    LogUnindent();

                std::string uuidStr = TrimWhitespace(line.substr(7)); // Skip "entity "
                EntityID uuid = EntityID::FromString(uuidStr);

                currentEntity = scene.AddEntity(uuid);

                LogInfo("Created entity '%s'\n", uuidStr.c_str());
                LogIndent();
            }
            else if (line == "assets:") {
                isInAssetSection = true;

                if (currentEntity) {
                    currentEntity = nullptr;
                    LogUnindent();
                }

                LogInfo("Parsing asset section\n");
                LogIndent();
            }
            else if (line.find("version:") == 0) {   // unused
                auto [key, value] = ParseKeyValuePair(line);
                LogInfo("Scene version: '%s'\n", value.c_str());
            }
            else if (line.find("scene:") == 0) {     // unused
                auto [key, value] = ParseKeyValuePair(line);
                LogInfo("Scene name: '%s'\n", value.c_str());
            }
        }
        else if (indent == 4) {
            if (isInAssetSection) {
                if (line.front() == '"' && line.back() == '"')
                    line = line.substr(1, line.length() - 2);

                AssetID uuid = AssetID::FromString(line);
                if (uuid.IsValid()) {
                    assets.push_back(uuid);
                    LogInfo("Found asset '%s'\n", line.c_str());
                }
            }
            else if (line.find("component ") == 0) {
                if (currentComponent && !currentComponentType.empty())
                    currentComponent->Reflect(&fieldWriter);

                currentComponentType = TrimWhitespace(line.substr(10)); // Skip "component "

                auto &registry = ComponentRegistry::GetMap();
                auto iter = registry.find(currentComponentType);
                if (iter != registry.end()) {
                    currentComponent = iter->second.createFunc(currentEntity, true);
                    currentEntity->AddComponentRaw(currentComponent);
                    fieldWriter = FieldWriter();

                    LogInfo("Added component '%s'\n", currentComponentType.c_str());
                }
                else {
                    LogWarn("Unknown component type '%s' at line %d\n", currentComponentType.c_str(), lineNumber);
                    currentComponent = nullptr;
                    currentComponentType.clear();
                }
            }
            else {
                if (currentEntity) {
                    auto [key, value] = ParseKeyValuePair(line);

                    if (key == "name") { // unused

                    }
                    else if (key == "isActive") {
                        currentEntity->isActive = (value == "true" || value == "True" || value == "1");
                    }
                    else if (key == "parent") { // unused

                    }
                }
            }
        }
        else if (indent == 8) {
            if (currentComponent) {
                auto [key, value] = ParseKeyValuePair(line);

                if (key == "isActive") {
                    currentComponent->isActive = (value == "true" || value == "True" || value == "1");
                }
                else {
                    fieldWriter.SetFieldValue(key, value);
                }
            }
        }
    }

    if (currentComponent && !currentComponentType.empty())
        currentComponent->Reflect(&fieldWriter);

    if (currentEntity)
        LogUnindent();

    file.close();

    if (this->assetManager) {
        for (const AssetID &uuid : assets)
            this->assetManager->MarkAsUsed(uuid);

        this->assetManager->CleanUpUnused();
    }

    LogUnindent();
    LogInfo("Scene loaded successfully!\n");

    return true;
}