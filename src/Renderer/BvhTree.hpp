#pragma once
#include <src/Graphics/Camera.hpp>
#include <src/Scene/Scene.hpp>

namespace devs_out_of_bounds {
class BvhTree : NoCopy, NoMove {
public:
    struct Tree {
        AABB aabb;
        int32_t object_index;
        int32_t right_child = -1;
        int32_t left_child = -1;
    };

    BvhTree(const std::vector<Actor>& actors);
    ~BvhTree();

    template <typename TCallable>
    void QueryCandidates( TCallable&& fn) const {
        QueryCandidatesInternal(fn, m_root);
    }

private:
    template <typename TCallable>
    void QueryCandidatesInternal(TCallable&& fn, const Tree& tree) const {
       
    }

    void Build();

private:
    Tree m_root;
    std::vector<Tree> m_trees;

    std::vector<Actor> m_actors;
};
} // namespace devs_out_of_bounds