#pragma once
#include <src/Graphics/Camera.hpp>
#include <src/Scene/Scene.hpp>

namespace devs_out_of_bounds {
class PathTracer : NoCopy, NoMove {
public:
    PathTracer();
    ~PathTracer();

    void OnResize(int new_width, int new_height);
    void OnUpdate(float frame_time);

    DOOB_NODISCARD Pixel Evaluate(int x, int y) const;

private:
    void RebuildAccelerationStructures();

    void LoadScene();
    void Cleanup();


private:
    ActorId m_light_actor;
    ActorId m_plane_actor;

    Scene* m_scene = nullptr;

    glm::vec2 m_inv_width_height = { 1.0f, 1.0f };
    float m_ar = 1.0f;

    Camera m_camera = {};
};
} // namespace devs_out_of_bounds