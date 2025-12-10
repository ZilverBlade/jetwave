#pragma once
#include <src/Scene/Actor.hpp>

#include <algorithm>
#include <vector>

namespace devs_out_of_bounds {
class Scene : NoCopy, NoMove {
public:
    Scene() {}
    ~Scene() {}

    DOOB_NODISCARD ActorId NewDrawableActor(IShape* shape, IMaterial* material);
    DOOB_NODISCARD ActorId NewLightActor(ILight* light);

    void DeleteActor(ActorId id);

    template <typename TCallable>
    void QueryScene(TCallable&& fn) const {
        for (ActorId id : m_alive) {
            size_t index = static_cast<size_t>(id.id) - 1;
            assert(index < m_actors.size() && "Bad index!");
            const Actor& cref = m_actors[index];
            fn(cref);
        }
    }

    DOOB_NODISCARD Actor& GetActorById(ActorId id);

private:
    DOOB_NODISCARD ActorId AllocateActor(Actor** out_actor);

    std::vector<Actor> m_actors = {};
    std::vector<ActorId> m_alive = {};
    std::vector<ActorId> m_tombstones = {};
};
} // namespace devs_out_of_bounds