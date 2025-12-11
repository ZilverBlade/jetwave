#pragma once
#include <src/Graphics/Camera.hpp>
#include <src/Scene/Scene.hpp>

namespace devs_out_of_bounds {
struct PathTracerParameters {
    int max_light_bounces = 1;
    bool b_gt7_tonemapper = false;
    bool b_accumulate = false;
};
class PathTracer : NoCopy, NoMove {
public:
    PathTracer();
    ~PathTracer();

    void OnResize(int new_width, int new_height);
    void OnUpdate(float frame_time);

    DOOB_NODISCARD Pixel Evaluate(int x, int y, uint32_t seed) const;

public:
    // Parameters
    PathTracerParameters m_parameters = {};


private:
    void RebuildAccelerationStructures();

    void LoadScene();
    void Cleanup();

    DOOB_NODISCARD bool IntersectFirstActor(
        const Ray& ray, Intersection* /*nullable*/ out_intersection, DrawableActor* /*nullable*/ out_actor) const;
    DOOB_NODISCARD bool IntersectAnyActor(const Ray& ray) const;
    DOOB_NODISCARD glm::vec3 ShadeActor(
        const DrawableActor& actor, const LightInput& shading_input, const Intersection& intersection, uint32_t& seed) const;
    DOOB_NODISCARD glm::vec3 SampleSky(const glm::vec3& R) const;

private:
    // Scene
    ActorId m_plane_actor;
    ActorId m_light_actor;

    std::vector<DrawableActor> m_drawable_actors = {};
    std::vector<LightActor> m_light_actors = {};

    Scene* m_scene = nullptr;

private:
    mutable std::vector<glm::dvec3> m_accumulator = {};
    mutable uint32_t m_accumulation_count = 1;

private:
    // Camera
    glm::vec2 m_inv_width_height = { 1.0f, 1.0f };
    int m_width = 1;
    float m_ar = 1.0f;

    Camera m_camera = {};
};
} // namespace devs_out_of_bounds