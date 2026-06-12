#ifndef IMGUI_INSPECTOR_HPP
#define IMGUI_INSPECTOR_HPP

#include "scene/component_registry.hpp"
#include "resources/asset_manager.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"

#include <string>
#include <commdlg.h>
#include <filesystem>

namespace fs = std::filesystem;

struct ImGuiInspector : public ComponentRegistry::Inspector {
private:
    AssetManager *assetManager;

    static std::string PickMetaFile(const std::string &initialDir) {
        OPENFILENAMEA openFileName{};
        char path[MAX_PATH] = {};

        openFileName.lStructSize = sizeof(openFileName);
        openFileName.lpstrFile = path;
        openFileName.nMaxFile = MAX_PATH;
        openFileName.lpstrFilter = "Asset meta files\0*.meta\0All files\0*.*\0";
        openFileName.nFilterIndex = 1;
        openFileName.lpstrInitialDir = initialDir.c_str();
        openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&openFileName))
            return path;

        return {};
    }

public:
    ImGuiInspector(AssetManager *assetManager) : assetManager(assetManager) {}

    bool Field(const std::string &name, int &val) override {
        return ImGui::DragInt(name.c_str(), &val);
    }

    bool Field(const std::string &name, unsigned int &val) override {
        int temp = val;
        if (ImGui::DragInt(name.c_str(), &temp, 1.0f, 0, INT_MAX)) {
            val = temp;
            return true;
        }

        return false;
    }

    bool Field(const std::string &name, float &val) override {
        return ImGui::DragFloat(name.c_str(), &val, 0.01f);
    }

    bool Field(const std::string &name, bool &val) override {
        return ImGui::Checkbox(name.c_str(), &val);
    }

    bool Field(const std::string &name, std::string &val) override {
        return ImGui::InputText(name.c_str(), &val);
    }

    bool Field(const std::string &name, XMFLOAT2 &val) override {
        return ImGui::DragFloat2(name.c_str(), &val.x, 0.01f);
    }

    bool Field(const std::string &name, XMFLOAT3 &val) override {
        return ImGui::DragFloat3(name.c_str(), &val.x, 0.01f);
    }

    bool Field(const std::string &name, XMFLOAT4 &val) override {
        return ImGui::DragFloat4(name.c_str(), &val.x, 0.01f);
    }

    bool Field(const std::string &name, UUID_ &val) override {
        bool didChange = false;

        ImGui::Text("%s", name.c_str());
        ImGui::Indent();

        std::string pathDisplay;
        if (this->assetManager && val.IsValid()) {
            pathDisplay = this->assetManager->UUIDToPath(val);
            if (pathDisplay.empty())
                pathDisplay = "(unknown)";
        }
        else {
            pathDisplay = val.IsValid() ? "(unknown)" : "(invalid)";
        }

        float browseWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseWidth - ImGui::GetStyle().ItemSpacing.x);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        ImGui::InputText(("##path_" + name).c_str(), &pathDisplay, ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button(("...##" + name).c_str())) {
            std::string dir = this->assetManager ? this->assetManager->GetAssetDirectory() : "assets/";
            std::string pickedFile = PickMetaFile(dir);
            if (!pickedFile.empty()) {
                fs::path assetPath(pickedFile);
                if (assetPath.extension() == ".meta")
                    assetPath = assetPath.parent_path() / assetPath.stem();

                fs::path relativePath = assetPath.lexically_relative(fs::absolute(dir));

                AssetID newID = this->assetManager->PathToUUID(relativePath.generic_string());
                if (newID.IsValid()) {
                    val = newID;
                    didChange = true;
                }
                else {
                    LogWarn("No registered asset found for '%s'\n", relativePath.generic_string().c_str());
                }
            }
        }

        std::string uuidDisplay = val.IsValid() ? val.ToString() : "(invalid)";
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        ImGui::InputText(("##uuid_" + name).c_str(), &uuidDisplay, ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();

        ImGui::Unindent();
        return didChange;
    }
};

#endif