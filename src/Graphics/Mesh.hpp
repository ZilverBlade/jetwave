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
    MeshInstance(const Mesh* mesh, const glm::mat4& transform = glm::mat4(1.0f)) {
        m_positions.reserve(mesh->GetVertices().size());
        m_attributes.reserve(mesh->GetVertices().size());

        m_index_ptr = mesh->GetIndices().data();
        m_num_indices = mesh->GetIndices().size();

        for (int i = 0; i < mesh->GetVertices().size(); ++i) {
            glm::vec4 pos_h = glm::vec4(mesh->GetVertices()[i].position, 1.0f);
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

    DOOB_NODISCARD AABB GetPrimitiveAabb(uint32_t primitive) const {
        glm::vec3 a = m_positions[m_index_ptr[primitive * 3 + 0]];
        glm::vec3 b = m_positions[m_index_ptr[primitive * 3 + 1]];
        glm::vec3 c = m_positions[m_index_ptr[primitive * 3 + 2]];
        AABB aabb;
        aabb.min = glm::min(a, glm::min(b, c));
        aabb.max = glm::max(a, glm::max(b, c));
        return aabb;
    }


    DOOB_NODISCARD Fragment SampleFragment(const Intersection& intersection) const {
        float v = intersection.barycentric.s;
        float w = intersection.barycentric.t;
        float u = 1.0f - v - w;

        uint32_t i, j, k;
        i = m_index_ptr[intersection.primitive * 3 + 0];
        j = m_index_ptr[intersection.primitive * 3 + 1];
        k = m_index_ptr[intersection.primitive * 3 + 2];

        return {
            .position = intersection.position, // u * m_positions[i] + v * m_positions[j] + w * m_positions[k],
            .normal = u * m_attributes[i].normal + v * m_attributes[j].normal + w * m_attributes[k].normal,
            .flat_normal = intersection.flat_normal,
            .tangent = u * m_attributes[i].tangent + v * m_attributes[j].tangent + w * m_attributes[k].tangent,
            .uv = u * m_attributes[i].uv + v * m_attributes[j].uv + w * m_attributes[k].uv,

            .b_front_face = intersection.b_front_facing != 0,
        };
    }

    const uint32_t* m_index_ptr;
    uint32_t m_num_indices;
    std::vector<glm::vec3> m_positions;
    std::vector<VertexAttributes> m_attributes;
};
} // namespace devs_out_of_bounds