#pragma once
#include <src/Graphics/ILight.hpp>
#include <src/Graphics/IMaterial.hpp>
#include <src/Graphics/IShape.hpp>

namespace devs_out_of_bounds {
struct ActorId {
    friend auto operator<=>(const ActorId&, const ActorId&) = default;
    uint32_t id = 1;
    DOB_NODISCARD DOB_FORCEINLINE bool Valid() const { return id > 0; }
};
class Actor {
public:
    Actor() = default;
    Actor(ActorId id) : m_id(id) {}

    DOB_NODISCARD DOB_FORCEINLINE bool Valid() const { return m_id.Valid(); }
    DOB_NODISCARD DOB_FORCEINLINE ActorId GetId() const { return m_id; }

    DOB_NODISCARD DOB_FORCEINLINE IShape* GetShape() const { return m_shape; }
    DOB_NODISCARD DOB_FORCEINLINE IMaterial* GetMaterial() const { return m_material; }
    DOB_NODISCARD DOB_FORCEINLINE ILight* GetLight() const { return m_light; }
    DOB_FORCEINLINE void SetShape(IShape* shape) { m_shape = shape; }
    DOB_FORCEINLINE void SetMaterial(IMaterial* material) { m_material = material; }
    DOB_FORCEINLINE void SetLight(ILight* light) { m_light = light; }

    friend auto operator<=>(const Actor& a, const Actor& b) { return a.m_id <=> b.m_id; }

private:
    IShape* m_shape = nullptr;
    IMaterial* m_material = nullptr;
    ILight* m_light = nullptr;
    ActorId m_id = {};
};
} // namespace devs_out_of_bounds