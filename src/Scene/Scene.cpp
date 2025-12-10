#include "Scene.hpp"

namespace devs_out_of_bounds {
ActorId Scene::NewActor(IShape* shape, IMaterial* material) {
    ActorId id = {};
    if (m_tombstones.empty()) {
        m_actors.push_back({});
        id.id = static_cast<uint32_t>(m_actors.size());
    } else {
        id = m_tombstones.back();
        m_tombstones.pop_back();
    }
    size_t index = static_cast<size_t>(id.id) - 1;
    m_actors[index] = Actor(id);
    m_actors[index].SetShape(shape);
    m_actors[index].SetMaterial(material);

    m_alive.emplace_back(id);

    return id;
}
void Scene::DeleteActor(ActorId id) {
    size_t index = static_cast<size_t>(id.id) - 1;
    assert(index < m_actors.size() && "Bad index!");
    m_tombstones.push_back(id);
    auto it = std::find(m_alive.begin(), m_alive.end(), id);
    assert(it != m_alive.end() && "Double free!");
    m_alive.erase(it);
}
} // namespace devs_out_of_bounds