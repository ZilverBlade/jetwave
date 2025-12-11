#pragma once
#include <src/Graphics/IShape.hpp>
#include <vector>

namespace devs_out_of_bounds {
class Trimesh : public IShape {
public:
    Trimesh(const std::vector<glm::vec3>& positions, const glm::mat4& transform = glm::mat4(1.0f))
        : m_positions(positions) {
        for (int i = 0; i < m_positions.size(); ++i) {
            glm::vec4 pos_h = glm::vec4(m_positions[i], 1.0f);
            m_positions[i] = glm::vec3(transform * pos_h);
        }
    }

    std::vector<glm::vec3> m_positions;
};

class TrimeshRange : public IShape {
public:
    Trimesh(Trimesh* trimesh, const std::vector<uint32_t>& indices) : m_trimesh(trimesh), m_indices(indices) {}

    DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
        auto& positions = m_trimesh->m_positions;
        for (size_t i = 0; i < m_indices.size(); i += 3) {
            glm::vec3 a = positions[i];
            glm::vec3 b = positions[i + 1];
            glm::vec3 c = positions[i + 2];
        }
        return false;
    }

    Trimesh* m_trimesh;
    std::vector<uint32_t> m_indices;
};
} // namespace devs_out_of_bounds