#include "PathTracer.hpp"
#include "Tonemapping.hpp"
#include <random>
#include <src/Graphics/Lights/AreaLight.hpp>
#include <src/Graphics/Lights/DirectionalLight.hpp>
#include <src/Graphics/Lights/PointLight.hpp>
#include <src/Graphics/Materials/BasicMaterial.hpp>
#include <src/Graphics/Materials/GridCutoutMaterial.hpp>
#include <src/Graphics/Materials/GridMaterial.hpp>
#include <src/Graphics/Shapes/Plane.hpp>
#include <src/Graphics/Shapes/Sphere.hpp>
#include <src/Graphics/Shapes/Triangle.hpp>

#include <SDL3/SDL.h>

namespace devs_out_of_bounds {

static material::BasicMaterial g_default_mat;
static material::GridMaterial g_grid_mat;
static material::GridCutoutMaterial g_cookie_mat;
static std::vector<light::PointLight> g_point_lights;

constexpr float CAMERA_FOV_DEG = 90.0f;

PathTracer::PathTracer() {
    m_scene = new Scene();

    LoadScene();
    BakeScene();
}

PathTracer::~PathTracer() {
    Cleanup();
    delete m_scene;
}

void PathTracer::OnResize(int new_width, int new_height) {
    assert(new_width > 0 && new_height > 0 && "Bad width and height!");
    float inv_width = 1.0f / static_cast<float>(new_width);
    float inv_height = 1.0f / static_cast<float>(new_height);
    m_inv_width_height = { inv_width, inv_height };
    m_ar = static_cast<float>(new_width) * inv_height;
    m_accumulator.resize(static_cast<size_t>(new_width) * new_height);
    m_width = new_width;
    ResetAccumulator();
}

void PathTracer::OnUpdate(float frame_time) {
    static float accum = 0.0f;

    { // INPUT
        bool b_moved_camera = false;
        Camera& camera = m_parameters.assets.camera;
        const bool* state = SDL_GetKeyboardState(nullptr);

        glm::vec3 camera_move = {};

        float cos_pitch = cos(m_camera_pitch);
        float sin_pitch = sin(m_camera_pitch);
        float cos_yaw = cos(m_camera_yaw);
        float sin_yaw = sin(m_camera_yaw);

        glm::vec3 forward = { -sin_yaw * cos_pitch, sin_pitch, cos_yaw * cos_pitch };
        glm::vec3 right = { cos_yaw, 0.0f, sin_yaw };
        glm::vec3 up = { 0, 1, 0 };

        if (state[SDL_SCANCODE_W]) {
            b_moved_camera = true;
            camera_move += forward;
        }
        if (state[SDL_SCANCODE_A]) {
            b_moved_camera = true;
            camera_move -= right;
        }
        if (state[SDL_SCANCODE_S]) {
            b_moved_camera = true;
            camera_move -= forward;
        }
        if (state[SDL_SCANCODE_D]) {
            b_moved_camera = true;
            camera_move += right;
        }
        if (state[SDL_SCANCODE_SPACE]) {
            b_moved_camera = true;
            camera_move += up;
        }
        if (state[SDL_SCANCODE_LCTRL]) {
            b_moved_camera = true;
            camera_move -= up;
        }

        if (state[SDL_SCANCODE_LEFT]) {
            b_moved_camera = true;
            m_camera_yaw += frame_time;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            b_moved_camera = true;
            m_camera_yaw -= frame_time;
        }
        if (state[SDL_SCANCODE_UP]) {
            b_moved_camera = true;
            m_camera_pitch += frame_time;
        }
        if (state[SDL_SCANCODE_DOWN]) {
            b_moved_camera = true;
            m_camera_pitch -= frame_time;
        }
        m_camera_pitch = std::clamp(m_camera_pitch, -glm::half_pi<float>() + 0.001f, glm::half_pi<float>() - 0.001f);

        if (b_moved_camera) {
            this->ResetAccumulator();

            if (glm::dot(camera_move, camera_move) > std::numeric_limits<float>::epsilon()) {
                camera.SetPosition(camera.GetPosition() + 8.0f * glm::normalize(camera_move) * frame_time);
            }
            camera.LookDir(forward, CAMERA_FOV_DEG);
        }


        if (state[SDL_SCANCODE_MINUS]) {
            camera.SetLogExposure(camera.GetLogExposure() - 4.0f * frame_time);
            this->ResetAccumulator();
        }
        if (state[SDL_SCANCODE_EQUALS]) {
            camera.SetLogExposure(camera.GetLogExposure() + 4.0f * frame_time);
            this->ResetAccumulator();
        }
    }

    if (!m_parameters.b_accumulate) {
        m_accumulation_count = 1;
        int index = 0;
        for (auto& light : g_point_lights) {
            float phase = index / static_cast<float>(m_light_actors.size()) * glm::two_pi<float>();
            light.m_position = glm::vec3(4.0f * glm::cos(accum + phase), 2.0f, 4.0f * glm::sin(accum + phase) + 4.0f);
            ++index;
        }
        accum += frame_time;
    } else {
        ++m_accumulation_count;
    }
}


Pixel PathTracer::Evaluate(int x, int y, uint32_t& seed) const {
    const float jitter_x = RandomFloatAdv<UniformDistribution>(seed) - 0.5f;
    const float jitter_y = RandomFloatAdv<UniformDistribution>(seed) - 0.5f;
    const float px = static_cast<float>(x) + 0.5f + jitter_x;
    const float py = static_cast<float>(y) + 0.5f + jitter_y;

    glm::vec2 ndc;
    ndc.x = (px * m_inv_width_height.x) * 2.0f - 1.0f;
    ndc.y = (py * m_inv_width_height.y) * 2.0f - 1.0f;
    ndc.x *= m_ar;
    ndc.y = -ndc.y;

    Ray ray = m_parameters.assets.camera.GetRay(ndc);

    glm::vec3 final_color = TracePath(ray, seed);

    glm::dvec3& summed = m_accumulator[static_cast<size_t>(y) * m_width + x];
    if (m_parameters.b_accumulate) {
        summed += static_cast<glm::dvec3>(final_color);
    } else {
        summed = static_cast<glm::dvec3>(final_color);
    }

    glm::vec3 color_avg = static_cast<glm::vec3>(summed / static_cast<double>(m_accumulation_count));
    glm::vec3 hdr = color_avg * m_parameters.assets.camera.ComputeExposureFactor();

    if (m_parameters.b_gt7_tonemapper) {
        GT7ToneMapping TM;
        initializeAsSDR(TM);
        applyToneMapping(hdr, TM);
    } else {
        hdr = 1.0f - exp(-hdr);
    }
    glm::vec3 gamma_corrected = glm::pow(hdr, glm::vec3(1.0f / 2.2f));

    float state = glm::dot(glm::vec3(x, y, 0.23142151f), glm::vec3(43523.432532f, 2132.343f, 123.52122f));
    uint32_t pixel_seed = reinterpret_cast<uint32_t&>(state) & 0x7FFFFF;

    float r_dither_offset = .7f / 255.f * (RandomFloatAdv<UniformDistribution>(pixel_seed) - 0.5f);
    float g_dither_offset = .7f / 255.f * (RandomFloatAdv<UniformDistribution>(pixel_seed) - 0.5f);
    float b_dither_offset = .7f / 255.f * (RandomFloatAdv<UniformDistribution>(pixel_seed) - 0.5f);

    return DOOB_WRITE_PIXEL_F32(gamma_corrected.r + r_dither_offset, gamma_corrected.g + g_dither_offset,
        gamma_corrected.b + b_dither_offset, 1.0f);
}

void PathTracer::ResetAccumulator() {
    m_accumulation_count = 0;
    size_t s = m_accumulator.size();
    m_accumulator.clear();
    m_accumulator.resize(s);
}

glm::vec3 PathTracer::TracePath(Ray ray, uint32_t& seed) const {
    glm::vec3 radiance(0.0f);
    glm::vec3 throughput(1.0f);

    glm::vec3 max_radiance;
    if (m_parameters.b_radiance_clamping) {
        float exposure = m_parameters.assets.camera.ComputeExposureFactor();
        float dynamic_range_limit = 1.0f;
        max_radiance = glm::vec3(dynamic_range_limit / std::max(exposure, 1e-4f));
    } else {
        max_radiance = glm::vec3(INFINITY);
    }

    for (int bounce = 0; bounce <= m_parameters.max_light_bounces; ++bounce) {
        Intersection hit;
        DrawableActor actor;

        if (!IntersectScene(ray, &hit, &actor)) {
            radiance += throughput * SampleSky(ray.direction);
            break;
        }
        glm::vec3 V = -ray.direction;

        Fragment frag = actor.shape->SampleFragment(hit);
        glm::vec3 Lr = {};
        glm::vec3 Le = {};
        // avoid excessive heap allocations
        thread_local static BSDF bsdf;
        bsdf.Reset();
        actor.material->Evaluate(frag, &bsdf, &Le);
        if (bsdf.HasBxDF()) {
            Lr = glm::min(ComputeDirectLighting(hit, V, seed, &bsdf), max_radiance);
        }
        radiance += glm::min(throughput * (Lr + Le), max_radiance);

        // If we reached max bounces, stop here.
        if (bounce == m_parameters.max_light_bounces)
            break;

        float pdf = 0.0f;
        glm::vec3 wi;
        glm::vec3 f = bsdf.Sample_Evaluate(V, wi, seed, pdf);

        if (pdf < FLT_EPSILON || glm::all(glm::lessThan(f, glm::vec3(FLT_EPSILON))) || glm::any(glm::isnan(f))) {
            break;
        }
        throughput *= f / pdf;

        // Update Ray
        ray.origin = hit.position + (glm::sign(glm::dot(wi, hit.flat_normal)) * hit.flat_normal * 1e-6f);
        ray.direction = wi;

        // russian roulette termination
        if (bounce > 3) {
            float p = std::max(throughput.x, std::max(throughput.y, throughput.z));
            if (RandomFloatAdv<UniformDistribution>(seed) > p)
                break;
            throughput /= p; // MUST divide by survival probability to keep energy correct!
        }
    }

    return radiance;
}
glm::vec3 PathTracer::ComputeDirectLighting(
    const Intersection& hit, const glm::vec3& V, uint32_t& seed, BSDF* bsdf) const {
    glm::vec3 Ld(0.0f);
    glm::vec3 P = hit.position;

    for (const auto& light_actor : m_light_actors) {
        LightSample sample = light_actor.light->Sample(P, seed);
        glm::vec3 Lt = CalcShadowTransmission({
            .origin = P,
            .t_min = 0.001f,
            .direction = sample.L,
            .t_max = sample.dist,
        });

        if (glm::dot(Lt, Lt) > 0.0f) {
            Ld += Lt * sample.Li * bsdf->Evaluate(V, glm::normalize(sample.L + V), sample.L);
        }
    }
    return Ld;
}
bool PathTracer::IntersectScene(Ray ray, Intersection* out_intersection, DrawableActor* out_actor) const {
    Intersection closest_hit = { .t = INFINITY };
    DrawableActor closest_actor;
    bool b_hit_something = false;

    for (const auto& actor : m_drawable_actors) {
        Intersection curr_intersection;
        if (!actor.shape->Intersect(ray, &curr_intersection)) {
            continue;
        }
        if (curr_intersection.t >= closest_hit.t) {
            continue;
        }

        closest_hit = curr_intersection;
        closest_actor = actor;
        b_hit_something = true;
    }

    if (b_hit_something) {
        if (out_intersection)
            *out_intersection = closest_hit;
        if (out_actor)
            *out_actor = closest_actor;
        return true;
    }
    return false;
}

glm::vec3 PathTracer::CalcShadowTransmission(Ray ray) const {
    glm::vec3 throughput(1.0f); // Start with full light
    const int max_transparent_hits = 8;


    for (int step = 0; step < max_transparent_hits; ++step) {
        Intersection closest_hit = { .t = INFINITY };
        DrawableActor closest_actor = {};
        bool b_hit_something = false;

        for (const auto& actor : m_drawable_actors) {
            Intersection curr_hit;
            if (actor.shape->Intersect(ray, &curr_hit)) {
                if (curr_hit.t < closest_hit.t) {
                    closest_hit = curr_hit;
                    closest_actor = actor;
                    b_hit_something = true;
                }
            }
        }

        if (!b_hit_something || closest_hit.t > ray.t_max) {
            return throughput;
        }

        thread_local static BSDF shadow_bsdf;
        shadow_bsdf.Reset();
        glm::vec3 temp_emission;
        Fragment frag = closest_actor.shape->SampleFragment(closest_hit);

        closest_actor.material->Evaluate(frag, &shadow_bsdf, &temp_emission);

        // Check forward transmission (wo = -ray, wi = ray)
        // "How much light passes straight through?"
        glm::vec3 tr = shadow_bsdf.Evaluate(-ray.direction, ray.direction, ray.direction);

        // TODO: maybe do russian roulette here instead for very low transmissions?
        if (glm::length(tr) < 0.001f) {
            return glm::vec3(0.0f);
        }

        throughput *= tr;

        ray.origin = closest_hit.position + (ray.direction * 1e-6f);
        ray.t_max -= closest_hit.t;
    }

    return glm::vec3(0.0f); // Too many layers, assume blocked
}

glm::vec3 PathTracer::SampleSky(const glm::vec3& direction) const {
    glm::vec3 sky_color = { 1, 1, 1 };
    if (m_parameters.assets.sky.skybox_texture) {
        sky_color = m_skybox_sampler.SampleCube(m_parameters.assets.sky.skybox_texture, direction);
    }
    return m_parameters.assets.sky.lux * m_parameters.assets.sky.skybox_tint * sky_color;
}

void PathTracer::RebuildAccelerationStructures() {}
void PathTracer::Cleanup() { m_parameters.assets.Clear(); }

void PathTracer::LoadScene() { SceneLoader::Load("assets/scenes/test-scene.json", *m_scene, m_parameters.assets); }
void PathTracer::BakeScene() {
    m_drawable_actors.clear();
    m_light_actors.clear();
    m_scene->QueryScene([this](const Actor& actor) {
        if (actor.GetShape() && actor.GetMaterial()) {
            m_drawable_actors.push_back(DrawableActor{ .shape = actor.GetShape(), .material = actor.GetMaterial() });
        }
        if (actor.GetLight()) {
            m_light_actors.push_back(LightActor{ .light = actor.GetLight() });
        }
    });
}

} // namespace devs_out_of_bounds