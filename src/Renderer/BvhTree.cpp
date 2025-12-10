#include "BvhTree.hpp"

namespace devs_out_of_bounds {
BvhTree::BvhTree(const std::vector<Actor>& actors) : m_actors(actors) {}
BvhTree::~BvhTree() {}

void BvhTree::Build() {
    for (int32_t i = 0; i < static_cast<int32_t>(m_actors.size()); ++i) {
        AABB aabb = m_actors[i].GetShape()->GetAABB();
    }
}
} // namespace devs_out_of_bounds