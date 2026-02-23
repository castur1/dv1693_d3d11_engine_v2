#ifndef RENDER_QUEUE_HPP
#define RENDER_QUEUE_HPP

#include "rendering/render_commands.hpp"

#include <vector>

class RenderQueue {
public:
    std::vector<GeometryCommand> geometryCommands;

    void Clear() {
        this->geometryCommands.clear();
    }

    void Submit(const GeometryCommand &command) {
        this->geometryCommands.push_back(command);
    }
};

#endif