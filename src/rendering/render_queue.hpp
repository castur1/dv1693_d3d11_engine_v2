#ifndef RENDER_QUEUE_HPP
#define RENDER_QUEUE_HPP

#include "rendering/render_commands.hpp"

#include <vector>

class RenderQueue {
public:
    std::vector<Geometry_command> geometryCommands;
    std::vector<Spot_light_command> spotLightCommands;
    std::vector<Directional_light_command> directionalLightCommands;

    void Clear() {
        this->geometryCommands.clear();
        this->spotLightCommands.clear();
        this->directionalLightCommands.clear();
    }

    void Submit(const Geometry_command &command) {
        this->geometryCommands.push_back(command);
    }

    void Submit(const Spot_light_command &command) {
        this->spotLightCommands.push_back(command);
    }

    void Submit(const Directional_light_command &command) {
        this->directionalLightCommands.push_back(command);
    }
};

#endif