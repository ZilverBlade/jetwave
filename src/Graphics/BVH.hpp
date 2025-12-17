#pragma once

#include <src/Core.hpp>
#include <src/Graphics/AABB.hpp>
#include <src/Graphics/Shapes/Trimesh.hpp>
#include <vector>
#include <deque>

namespace devs_out_of_bounds {

struct BvhNode {
    AABB aabb = {};
    uint32_t left = 0; // nodes can never point to the root so 0 is assumed to be NULL
    uint32_t right = 0;
    IShape* leaf = nullptr;
};
class BVH : public IShape {
public:
    BVH(const MeshInstance* instance) {}

    DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
        std::deque<BvhNode*> m_remaining_intersections;
        m_remaining_intersections.push_back(m_nodes.begin());
        
        float closest_t = INFINITY; 
        while (m_remaining_intersections.size() > 0) {
            BvhNode* node = m_remaining_intersections.front();
            float t;
            if (node->aabb.RayIntersects(ray, &t) && t < closest_t) {
                t = closest_t;
                if (node->left > 0) {
                    m_remaining_intersections.push_back(m_nodes[node->left]);
                }
                if (node->right > 0) {
                    m_remaining_intersections.push_back(m_nodes[node->right]);
                }
                if (node->leaf) {
                    node->leaf->Intersect(ray, )
                    if (closest_t)
                }
            }
        }
        return false;
    }
    DOOB_NODISCARD AABB GetAABB() const override { return m_nodes.empty() ? {} : m_nodes[0]; }
    DOOB_NODISCARD Fragment SampleFragment(const Intersection& intersection) const override {
        return {
            .position = intersection.position,
            .normal = intersection.flat_normal,
            .tangent = {},
            .uv = {},
        };
    }


    glm::vec3 m_min;
    glm::vec3 m_max;

private:
    std::vector<shape::Trimesh> m_nodes;
    std::vector<BvhNode> m_nodes;
    const MeshInstance* m_instance;
};

} // namespace devs_out_of_bounds