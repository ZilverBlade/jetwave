#pragma once
#include <src/Graphics/Camera.hpp>
#include <src/Graphics/SamplerStates/SkyboxSampler.hpp>
#include <src/Graphics/TextureViews/Rgbe9TextureView.hpp>
#include <src/Scene/Scene.hpp>

namespace devs_out_of_bounds {

struct PathTracerParameters {
    int max_light_bounces = 0;
    bool b_gt7_tonemapper = false;
    bool b_accumulate = false;
    float m_log_camera_exposure = 0.0f;
};

class PathTracer : NoCopy, NoMove {
public:
    PathTracer();
    ~PathTracer();

    void OnResize(int new_width, int new_height);
    void OnUpdate(float frame_time);

    DOOB_NODISCARD Pixel Evaluate(int x, int y, uint32_t seed) const;

    void ResetAccumulator();
    uint32_t GetSamplesAccumulated() const { return m_accumulation_count; }

public:
    PathTracerParameters m_parameters = {};

private:
    void RebuildAccelerationStructures();
    void LoadScene();
    void BakeScene();
    void Cleanup();

    // --- Core Path Tracing Logic ---

    // Solves the rendering equation iteratively
    DOOB_NODISCARD glm::vec3 TracePath(Ray ray, uint32_t& seed) const;

    // Calculates L_dir (Direct Light + Shadow Rays)
    DOOB_NODISCARD void ComputeDirectLighting(const DrawableActor& hit_actor, const Intersection& hit_info,
        const glm::vec3& view_dir, uint32_t& seed, glm::vec3& out_diffuse, glm::vec3& out_specular) const;

    // Helper to interact with the Scene
    DOOB_NODISCARD bool IntersectScene(const Ray& ray, Intersection* out_intersection, DrawableActor* out_actor) const;

    // Returns true if a ray hits ANYTHING (for shadows)
    DOOB_NODISCARD bool IsOccluded(const Ray& ray) const;

    DOOB_NODISCARD glm::vec3 SampleSky(const glm::vec3& direction) const;

private:
    uint8_t* m_skybox_texture_data = nullptr;
    Rgbe9TextureView m_skybox_texture{ nullptr, 0, 0, 0 };
    SkyboxSampler m_skybox_sampler;

private:
    // Scene Data
    ActorId m_plane_actor;
    ActorId m_light_actor;

    std::vector<DrawableActor> m_drawable_actors = {};
    std::vector<LightActor> m_light_actors = {};

    Scene* m_scene = nullptr;

    // Accumulator
    mutable std::vector<glm::dvec3> m_accumulator = {};
    mutable uint32_t m_accumulation_count = 1;

    // Camera
    glm::vec2 m_inv_width_height = { 1.0f, 1.0f };
    int m_width = 1;
    float m_ar = 1.0f;
    Camera m_camera = {};

    glm::vec3 m_camera_position = {};
    float m_camera_pitch = 0.0f;
    float m_camera_yaw = 0.0f;
};
} // namespace devs_out_of_bounds