#pragma once
#include <src/Graphics/IShape.hpp>
#include <vector>

namespace devs_out_of_bounds {
class Trimesh : public IShape {
public:
    Trimesh(const glm::vec3& center, const std::vector<glm::vec3>& positions)
        : m_center(center), m_positions(positions) {}

    glm::vec3 m_center;
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