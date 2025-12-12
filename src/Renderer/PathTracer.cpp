#include "PathTracer.hpp"
#include "Tonemapping.hpp"
#include <random>
#include <src/Graphics/Lights/AreaLight.hpp>
#include <src/Graphics/Lights/PointLight.hpp>
#include <src/Graphics/Materials/BasicMaterial.hpp>
#include <src/Graphics/Materials/GltfMaterial.hpp>
#include <src/Graphics/Materials/GridCutoutMaterial.hpp>
#include <src/Graphics/Materials/GridMaterial.hpp>
#include <src/Graphics/Materials/TriangleDebugMaterial.hpp>
#include <src/Graphics/Shapes/Plane.hpp>
#include <src/Graphics/Shapes/Sphere.hpp>
#include <src/Graphics/Shapes/Triangle.hpp>

#include <SDL3/SDL.h>
#include <stb_image.h>

namespace devs_out_of_bounds {

static BasicMaterial g_default_mat = BasicMaterial({ 1.0f, 1.0f, 1.0f }, { 1, 1, 1 }, 64.0f);
static TriangleDebugMaterial g_debug_mat = TriangleDebugMaterial();
static GridMaterial g_grid_mat = GridMaterial();
static GridCutoutMaterial g_cookie_mat = GridCutoutMaterial();
static std::vector<PointLight> g_point_lights;

constexpr float CAMERA_FOV_DEG = 90.0f;

PathTracer::PathTracer() {
    m_scene = new Scene();
    {
        int x, y, c;
        float* data = stbi_loadf("assets/citrus_orchard_road_puresky_2k.hdr", &x, &y, &c, 3);

        m_skybox_texture_data = new uint8_t[x * y * 4];
        for (int i = 0; i < x * y; ++i) {
            float* src_ptr = data + i * c;
            uint8_t* dst_ptr = m_skybox_texture_data + i * 4;

            float r = src_ptr[0];
            float g = src_ptr[1];
            float b = src_ptr[2];

            *reinterpret_cast<uint32_t*>(dst_ptr) = glm::packF3x9_E1x5({ r, g, b });
        }

        m_skybox_texture = Rgbe9TextureView(m_skybox_texture_data, x, y, x * 4);

        stbi_image_free(data);
    }

    LoadScene();
    BakeScene();
}

PathTracer::~PathTracer() {
    Cleanup();
    delete m_scene;
    delete[] m_skybox_texture_data;
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
        const bool* state = SDL_GetKeyboardState(nullptr);

        glm::vec3 camera_move = {};

        glm::vec3 forward = { -sin(m_camera_yaw), sin(m_camera_pitch), cos(m_camera_yaw) };
        glm::vec3 right = { cos(m_camera_yaw), 0.0f, sin(m_camera_yaw) };
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
        m_camera_pitch = std::clamp(m_camera_pitch, -glm::half_pi<float>() + 0.1f, glm::half_pi<float>() - 0.1f);

        if (b_moved_camera) {
            ResetAccumulator();

            if (glm::dot(camera_move, camera_move) > std::numeric_limits<float>::epsilon()) {
                m_camera.SetPosition(m_camera.GetPosition() + glm::normalize(camera_move) * frame_time);
            }
            m_camera.LookDir(forward, CAMERA_FOV_DEG);
        }


        if (state[SDL_SCANCODE_MINUS]) {
            m_parameters.m_log_camera_exposure -= 4.0f * frame_time;
        }
        if (state[SDL_SCANCODE_EQUALS]) {
            m_parameters.m_log_camera_exposure += 4.0f * frame_time;
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


Pixel PathTracer::Evaluate(int x, int y, uint32_t seed) const {
    const float jitter_x = RandomFloatAdv(seed) - 0.5f;
    const float jitter_y = RandomFloatAdv(seed) - 0.5f;
    const float px = static_cast<float>(x) + 0.5f + jitter_x;
    const float py = static_cast<float>(y) + 0.5f + jitter_y;

    glm::vec2 ndc;
    ndc.x = (px * m_inv_width_height.x) * 2.0f - 1.0f;
    ndc.y = (py * m_inv_width_height.y) * 2.0f - 1.0f;
    ndc.x *= m_ar;
    ndc.y = -ndc.y;

    Ray ray = m_camera.GetRay(ndc);

    glm::vec3 final_color = TracePath(ray, seed);

    glm::dvec3& summed = m_accumulator[static_cast<size_t>(y) * m_width + x];
    if (m_parameters.b_accumulate) {
        summed += static_cast<glm::dvec3>(final_color);
    } else {
        summed = static_cast<glm::dvec3>(final_color);
    }

    glm::vec3 color_avg = static_cast<glm::vec3>(summed / static_cast<double>(m_accumulation_count));
    glm::vec3 hdr = color_avg * expf(m_parameters.m_log_camera_exposure);

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

    float r_dither_offset = .3f / 255.f * (RandomFloatAdv(pixel_seed) - 0.5f);
    float g_dither_offset = .3f / 255.f * (RandomFloatAdv(pixel_seed) - 0.5f);
    float b_dither_offset = .3f / 255.f * (RandomFloatAdv(pixel_seed) - 0.5f);

    return DOOB_WRITE_PIXEL_F32(gamma_corrected.r + r_dither_offset, gamma_corrected.g + g_dither_offset,
        gamma_corrected.b + b_dither_offset, 1.0f);
}

void PathTracer::ResetAccumulator() {
    m_accumulation_count = 0;
    size_t s = m_accumulator.size();
    m_accumulator.clear();
    m_accumulator.resize(s);
}

// --- The Core Refactor: Path Logic ---

glm::vec3 PathTracer::TracePath(Ray ray, uint32_t& seed) const {
    glm::vec3 radiance(0.0f);
    glm::vec3 throughput(1.0f);

    for (int bounce = 0; bounce <= m_parameters.max_light_bounces; ++bounce) {
        Intersection hit;
        DrawableActor actor;

        if (!IntersectScene(ray, &hit, &actor)) {
            radiance += throughput * SampleSky(ray.direction);
            break;
        }

        glm::vec3 P = hit.t * ray.direction + ray.origin;
        glm::vec3 V = glm::normalize(ray.origin - P);

        Fragment frag = actor.shape->SampleFragment(hit);
        MaterialOutput mat = actor.material->Evaluate(frag);
        glm::vec3 N = mat.world_normal;

        glm::vec3 P_offset = P + (hit.flat_normal * 0.001f);

        glm::vec3 diffuse_light = {};
        glm::vec3 specular_light = {};

        ComputeDirectLighting(actor, hit, V, seed, diffuse_light, specular_light);

        float fresnel = glm::max(glm::dot(V, N), 0.0f);
        float f2 = fresnel * fresnel;
        float f4 = f2 * f2;
        float f5 = f4 * fresnel;
        float kD = f5;

        radiance += throughput * (diffuse_light * kD + specular_light * (1.0f - kD));

        // If we reached max bounces, stop here.
        if (bounce == m_parameters.max_light_bounces)
            break;


        glm::vec3 next_dir;
        bool is_specular_bounce = RandomFloatAdv(seed) < (fresnel * 0.5f);

        if (is_specular_bounce) {
            // Specular Reflection
            next_dir = glm::reflect(-V, N);
            throughput *= mat.specular_color;
        } else {
            glm::vec3 random_dir = glm::normalize(glm::vec3(RandomFloatAdv(seed) * 2.0f - 1.0f,
                RandomFloatAdv(seed) * 2.0f - 1.0f, RandomFloatAdv(seed) * 2.0f - 1.0f));
            next_dir = glm::normalize(N + random_dir);
            throughput *= mat.albedo_color;
        }

        // Update Ray for next iteration
        ray.origin = P_offset;
        ray.direction = next_dir;
    }

    return radiance;
}

void PathTracer::ComputeDirectLighting(const DrawableActor& actor, const Intersection& hit, const glm::vec3& V,
    uint32_t& seed, glm::vec3& out_diffuse, glm::vec3& out_specular) const {

    // Recalculate basic properties needed for light evaluation
    Fragment fragment = actor.shape->SampleFragment(hit);
    MaterialOutput material = actor.material->Evaluate(fragment);

    LightInput light_input;
    light_input.P = hit.position;
    light_input.eye = light_input.P + V;
    light_input.V = V;
    light_input.N = material.world_normal;

    for (const auto& light_actor : m_light_actors) {
        auto ShadowCheck = [](const Ray& shadow_ray, const void* userdata) -> bool {
            const PathTracer* pt = reinterpret_cast<const PathTracer*>(userdata);
            return !pt->IsOccluded(shadow_ray);
        };

        LightOutput result = light_actor.light->Evaluate(light_input, { .specular_power = material.specular_power },
            seed, { .userdata = this, .fn_shadow_check = ShadowCheck });

        out_diffuse += result.diffuse;
        out_specular += result.specular;
    }

    out_diffuse *= material.albedo_color;
    out_specular *= material.specular_color;
}

bool PathTracer::IntersectScene(const Ray& ray, Intersection* out_intersection, DrawableActor* out_actor) const {
    Intersection closest_hit{ .t = INFINITY };
    DrawableActor closest_actor;
    bool hit_something = false;

    for (const auto& actor : m_drawable_actors) {
        Intersection curr_intersection;
        if (actor.shape->Intersect(ray, &curr_intersection)) {
            if (actor.material->EvaluateDiscard(actor.shape->SampleFragment(curr_intersection))) {
                continue;
            }
            if (curr_intersection.t < closest_hit.t) {
                closest_hit = curr_intersection;
                closest_actor = actor;
                hit_something = true;
            }
        }
    }

    if (hit_something) {
        if (out_intersection)
            *out_intersection = closest_hit;
        if (out_actor)
            *out_actor = closest_actor;
        return true;
    }
    return false;
}

bool PathTracer::IsOccluded(const Ray& ray) const {
    for (const auto& actor : m_drawable_actors) {
        Intersection intersection;
        if (actor.shape->Intersect(ray, &intersection)) {
            if (actor.material->EvaluateDiscard(actor.shape->SampleFragment(intersection))) {
                continue;
            }
            return true;
        }
    }
    return false;
}

glm::vec3 PathTracer::SampleSky(const glm::vec3& direction) const {
    //static glm::vec3 ambient_color = { 0.51f, 0.53f, 0.54f };
    //float t = 0.5f * (direction.y + 1.0f);
    //return (1.0f - t) * glm::vec3(1.0f) + t * glm::vec3(0.5f, 0.7f, 1.0f) * 0.5f;

    return m_skybox_sampler.SampleCube(&m_skybox_texture, direction);
}

void PathTracer::RebuildAccelerationStructures() {}
void PathTracer::Cleanup() {}

void PathTracer::LoadScene() {
    static Plane plane1 = Plane({ 0, 1, 0 }, { 0, -1, 0 });
    m_plane_actor = m_scene->NewDrawableActor(&plane1, &g_grid_mat);

    static Sphere sphere1 = Sphere({ 0.0f, 0.0f, 5.0f }, 1.0f, true);
    static Sphere sphere1_back = Sphere({ 0.0f, 0.0f, 5.0f }, 1.0f, false);
    ActorId sphere_actor = m_scene->NewDrawableActor(&sphere1, &g_cookie_mat);
    ActorId sphere_actor_back = m_scene->NewDrawableActor(&sphere1_back, &g_cookie_mat);

    static Triangle triangle1 = Triangle({ 0, 0, 3 }, { 0, 3, 3 }, { 1, 1, 4 });

    std::vector<glm::vec3> light_colors{ { 1.f, 1.f, 1.f }, { 1.f, .1f, .1f }, { .1f, .1f, 1.f }, { .1f, 1.f, .1f },
        { 1.f, 1.f, .1f }, { .1f, 1.f, 1.f } };
    g_point_lights.clear();
    g_point_lights.reserve(light_colors.size());
    for (int i = 0; i < light_colors.size(); ++i) {
        g_point_lights.push_back(PointLight({ 0, 10, 0 }, light_colors[i] * 50.0f));
        m_light_actor = m_scene->NewLightActor(&g_point_lights.back());
    }

    // static AreaLight area_light1 = AreaLight({ 0, 4.0f, 8 }, { 1, 2 }, { 150, 150, 150 });
    // ActorId light_actor1 = m_scene->NewLightActor(&area_light1);
}
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