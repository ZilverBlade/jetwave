#pragma once

#include <src/Core.hpp>
#include <vector>


namespace devs_out_of_bounds {
struct ImageData {
    uint32_t width = {};
    uint32_t height = {};
    std::vector<uint8_t> data = {};
};

struct MeshData {
    std::vector<glm::vec3> positions = {};
    std::vector<glm::vec3> normals = {};
    std::vector<glm::vec4> tangents = {};
    std::vector<std::vector<glm::vec2>> uvs = {};
    std::vector<uint32_t> indices = {};
};
} // namespace devs_out_of_bounds