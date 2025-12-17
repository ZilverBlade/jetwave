#pragma once

#include <algorithm>
#include <src/Core.hpp>
#include <src/Graphics/AABB.hpp>
#include <src/Graphics/Shapes/Trimesh.hpp>
#include <stack>
#include <vector>

namespace devs_out_of_bounds {
namespace shape {

    struct BvhNode {
        AABB aabb = {};
        uint32_t left = 0; // nodes can never point to the root so 0 is assumed to be NULL
        uint32_t right = 0;
        int32_t leaf = -1;
    };
    class BVH : public IShape {
    public:
        static constexpr size_t MAX_PRIMITIVES_PER_LEAF = 8;

        static constexpr size_t MAX_DEPTH = 32;

        static constexpr int AXIS_X = 0;
        static constexpr int AXIS_Y = 1;
        static constexpr int AXIS_Z = 2;
        static constexpr int MAX_AXIS = 3;

        void BuildBvhRecursive(uint32_t node_index, const std::vector<uint32_t>& primitives, uint32_t start,
            uint32_t end, int depth, int axis) {
            if (start == end || primitives.empty())
                return;

            AABB aabb = {};
            aabb.min = { INFINITY, INFINITY, INFINITY };
            aabb.max = { -INFINITY, -INFINITY, -INFINITY };
            std::vector<float> centers;
            for (uint32_t i = start; i < end; ++i) {
                uint32_t prim_index = primitives[i];
                AABB prim_aabb = m_instance->GetPrimitiveAabb(prim_index);
                aabb = aabb.Union(m_instance->GetPrimitiveAabb(prim_index));
                centers.push_back((prim_aabb.min[axis] + prim_aabb.max[axis]) * 0.5f);
            }
            float median_along_axis = 0.0f;

            std::sort(centers.begin(), centers.end());
            if ((centers.size() % 2) == 0) {
                median_along_axis = (centers[centers.size() / 2] + centers[centers.size() / 2 + 1]) * 0.5f;
            } else {
                median_along_axis = centers[centers.size() / 2];
            }

            std::vector<uint32_t> left_primitives;
            std::vector<uint32_t> right_primitives;
            for (uint32_t i = start; i < end; ++i) {
                uint32_t prim_index = primitives[i];
                AABB prim_aabb = m_instance->GetPrimitiveAabb(prim_index);
                float prim_center = (prim_aabb.min[axis] + prim_aabb.max[axis]) * 0.5f;
                if (prim_center < median_along_axis) {
                    left_primitives.push_back(prim_index);
                } else {
                    right_primitives.push_back(prim_index);
                }
            }

            m_nodes[node_index].aabb = aabb;

            if (left_primitives.empty() || right_primitives.empty() || depth >= MAX_DEPTH ||
                primitives.size() < MAX_PRIMITIVES_PER_LEAF) {
                BuildBvhLeaf(node_index, primitives, start, end);
            } else {
                m_nodes[node_index].left = static_cast<uint32_t>(m_nodes.size());
                m_nodes.emplace_back();
                BuildBvhRecursive(m_nodes[node_index].left, left_primitives, 0,
                    static_cast<uint32_t>(left_primitives.size()), depth + 1, (axis + 1) % MAX_AXIS);

                m_nodes[node_index].right = static_cast<uint32_t>(m_nodes.size());
                m_nodes.emplace_back();
                BuildBvhRecursive(m_nodes[node_index].right, right_primitives, 0,
                    static_cast<uint32_t>(right_primitives.size()), depth + 1, (axis + 1) % MAX_AXIS);
            }
        }

        void BuildBvhLeaf(uint32_t node_index, const std::vector<uint32_t>& primitives, uint32_t start, uint32_t end) {
            std::vector<uint32_t> leaf_primitives;
            for (uint32_t i = start; i < end; ++i) {
                leaf_primitives.push_back(primitives[i]);
            }
            m_nodes[node_index].leaf = static_cast<int32_t>(m_leafs.size());
            m_leafs.emplace_back(m_instance, leaf_primitives);
        }


        BVH(const MeshInstance* instance) : m_instance(instance) {
            std::vector<uint32_t> all_primitives;
            all_primitives.reserve(instance->m_num_indices / 3);
            for (uint32_t i = 0; i < instance->m_num_indices; i += 3) {
                all_primitives.push_back(i);
            }
            m_nodes.reserve(all_primitives.size());
            m_nodes.emplace_back(); // root node
            BuildBvhRecursive(0, all_primitives, 0U, static_cast<uint32_t>(all_primitives.size()), 0, AXIS_X);
        }

        DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
            const BvhNode* stack[MAX_DEPTH + 1];
            size_t stack_ptr = 0;
            stack[stack_ptr++] = &m_nodes[0];

            uint32_t num_intersections = 0;

            float closest_prim_t = INFINITY;
            float closest_aabb_t = INFINITY;
            Intersection best_intersection;

            bool b_hit = false;
            while (stack_ptr > 0) {
                const BvhNode* node = stack[--stack_ptr];
                float t;
                if (node->aabb.RayIntersects(ray, &t) && t < closest_aabb_t) {
                    ++num_intersections;
                    closest_aabb_t = t;
                    if (node->left > 0 && stack_ptr < MAX_DEPTH) {
                        stack[stack_ptr++] = &m_nodes[node->left];
                    }
                    if (node->right > 0 && stack_ptr < MAX_DEPTH) {
                        stack[stack_ptr++] = &m_nodes[node->right];
                    }
                    if (node->leaf != -1) {
                        const shape::Trimesh& leaf_shape = m_leafs[node->leaf];
                        if (leaf_shape.Intersect(ray, &best_intersection) && best_intersection.t < closest_prim_t) {
                            num_intersections += best_intersection.num_intersections;
                            closest_prim_t = best_intersection.t;
                            b_hit = true;
                            if (out_intersection) {
                                *out_intersection = best_intersection;
                            }
                        }
                    }
                }
            }
            if (out_intersection) {
                out_intersection->num_intersections = num_intersections;
            }
            return b_hit;
        }
        DOOB_NODISCARD AABB GetAABB() const override { return m_nodes.empty() ? AABB{} : m_nodes[0].aabb; }
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
        std::vector<shape::Trimesh> m_leafs;
        std::vector<BvhNode> m_nodes;
        const MeshInstance* m_instance;
    };
} // namespace shape
} // namespace devs_out_of_bounds