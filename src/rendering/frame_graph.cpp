#include "frame_graph.hpp"

FrameGraph::FrameGraph() {}
FrameGraph::~FrameGraph() {}

void FrameGraph::SetDevice(ID3D11Device *device) {
    this->device = device;
}