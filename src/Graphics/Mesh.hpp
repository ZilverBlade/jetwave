#pragma once
#include <src/Graphics/Shapes/Triangle.hpp>
#include <vector>

namespace devs_out_of_bounds {
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;
};
class Mesh : NoCopy, NoMove {
public:
    DOOB_NODISCARD DOOB_FORCEINLINE const std::vector<uint32_t>& GetIndices() const { return m_indices; }
    DOOB_NODISCARD DOOB_FORCEINLINE const std::vector<Vertex>& GetVertices() const { return m_vertices; }

private:
    std::vector<uint32_t> m_indices;
    std::vector<Vertex> m_vertices;
};

struct VertexAttributes {
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;
};

class MeshInstance : NoCopy, NoMove {
public:
    MeshInstance(const Mesh* mesh, const glm::mat4& transform = glm::mat4(1.0f)) : {
        m_positions.reserve(mesh->GetVertices().size());
        m_attributes.reserve(mesh->GetVertices().size());

        for (int i = 0; i < mesh->GetVertices().size(); ++i) {
            glm::vec4 pos_h = glm::vec4(mesh->GetVertices()[i], 1.0f);
            m_positions.push_back(glm::vec3(transform * pos_h));
        }
        glm::mat3 normal_transform = glm::inverse(glm::transpose(glm::mat3(transform)));
        for (int i = 0; i < mesh->GetVertices().size(); ++i) {
            m_attributes.push_back({
                .normal = normal_transform * mesh->GetVertices()[i].normal,
                .tangent = normal_transform * mesh->GetVertices()[i].tangent,
                .uv = mesh->GetVertices()[i].uv,
            });
        }
    }

    std::vector<glm::vec3> m_positions;
    std::vector<VertexAttributes> m_attributes;
};
} // namespace devs_out_of_bounds