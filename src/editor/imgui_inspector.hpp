#ifndef IMGUI_INSPECTOR_HPP
#define IMGUI_INSPECTOR_HPP

#include "scene/component_registry.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"

#include <string>

class ImGuiInspector : public ComponentRegistry::Inspector {
    void Field(const std::string &name, int &val) override {
        ImGui::DragInt(name.c_str(), &val);
    }

    void Field(const std::string &name, unsigned int &val) override {
        int temp = val;
        if (ImGui::DragInt(name.c_str(), &temp, 1.0f, 0, INT_MAX))
            val = temp;
    }

    void Field(const std::string &name, float &val) override {
        ImGui::DragFloat(name.c_str(), &val, 0.01f);
    }

    void Field(const std::string &name, bool &val) override {
        ImGui::Checkbox(name.c_str(), &val);
    }

    void Field(const std::string &name, std::string &val) override {
        ImGui::InputText(name.c_str(), &val);
    }

    void Field(const std::string &name, XMFLOAT2 &val) override {
        ImGui::DragFloat2(name.c_str(), &val.x, 0.01f);
    }

    void Field(const std::string &name, XMFLOAT3 &val) override {
        ImGui::DragFloat3(name.c_str(), &val.x, 0.01f);
    }

    void Field(const std::string &name, XMFLOAT4 &val) override {
        ImGui::DragFloat4(name.c_str(), &val.x, 0.01f);
    }

    void Field(const std::string &name, UUID_ &val) override {
        ImGui::LabelText(name.c_str(), "%s", val.ToString().c_str()); // TODO: How should I handle assigning assets/entities?
    }
};

#endif