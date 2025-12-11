#include "PathTracer.hpp"
#include <src/Graphics/Lights/AreaLight.hpp>
#include <src/Graphics/Lights/PointLight.hpp>
#include <src/Graphics/Materials/BasicMaterial.hpp>
#include <src/Graphics/Materials/GridMaterial.hpp>
#include <src/Graphics/Materials/TriangleDebugMaterial.hpp>
#include <src/Graphics/Shapes/Plane.hpp>
#include <src/Graphics/Shapes/Sphere.hpp>

#include <random>
#include <src/Graphics/Shapes/Triangle.hpp>

namespace devs_out_of_bounds {
using namespace glm;


#define TONE_MAPPING_UCS_ICTCP 0
#define TONE_MAPPING_UCS_JZAZBZ 1
#define TONE_MAPPING_UCS TONE_MAPPING_UCS_ICTCP
#define GRAN_TURISMO_SDR_PAPER_WHITE 250.0f // cd/m^2
#define REFERENCE_LUMINANCE 100.0f          // cd/m^2 <-> 1.0f
#define exponentScaleFactor 1.0f
#define JZAZBZ_EXPONENT_SCALE_FACTOR 1.7f

float frameBufferValueToPhysicalValue(float fbValue) {
    // Converts linear frame-buffer value to physical luminance (cd/m^2)
    // where 1.0 corresponds to REFERENCE_LUMINANCE (e.g., 100 cd/m^2).
    return fbValue * REFERENCE_LUMINANCE;
}

float physicalValueToFrameBufferValue(float physical) {
    // Converts physical luminance (cd/m^2) to a linear frame-buffer value,
    // where 1.0 corresponds to REFERENCE_LUMINANCE (e.g., 100 cd/m^2).
    return physical / REFERENCE_LUMINANCE;
}

float smoothStep(float x, float edge0, float edge1) {
    float t = (x - edge0) / (edge1 - edge0);

    if (x < edge0) {
        return 0.0f;
    }
    if (x > edge1) {
        return 1.0f;
    }

    return t * t * (3.0f - 2.0f * t);
}

float chromaCurve(float x, float a, float b) { return 1.0f - smoothStep(x, a, b); }

// EOTF / inverse-EOTF for ST-2084 (PQ)

float eotfSt2084(float n, float exponent) {
    if (n < 0.0f) {
        n = 0.0f;
    }
    if (n > 1.0f) {
        n = 1.0f;
    }

    // Base functions from SMPTE ST 2084:2014
    // Converts from normalized PQ (0-1) to absolute luminance in cd/m^2 (linear light)
    // Assumes float input; does not handle integer encoding (Annex)
    // Assumes full-range signal (0-1)
    const float m1 = 0.1593017578125f; // (2610 / 4096) / 4
    float m2 = exponent * 78.84375f;   // (2523 / 4096) * 128
    const float c1 = 0.8359375f;       // 3424 / 4096
    const float c2 = 18.8515625f;      // (2413 / 4096) * 32
    const float c3 = 18.6875f;         // (2392 / 4096) * 32
    const float pqC = 10000.0f;        // Maximum luminance supported by PQ (cd/m^2)

    // Does not handle signal range from 2084 - assumes full range (0-1)
    float np = pow(n, 1.0f / m2);
    float l = np - c1;

    if (l < 0.0f) {
        l = 0.0f;
    }

    l = l / (c2 - c3 * np);
    l = pow(l, 1.0f / m1);

    // Convert absolute luminance (cd/m^2) into the frame-buffer linear scale.
    return physicalValueToFrameBufferValue(l * pqC);
}

float inverseEotfSt2084(float v, float exponent) {
    const float m1 = 0.1593017578125f;
    float m2 = exponent * 78.84375f;
    const float c1 = 0.8359375f;
    const float c2 = 18.8515625f;
    const float c3 = 18.6875f;
    const float pqC = 10000.0f;

    // Convert the frame-buffer linear scale into absolute luminance (cd/m^2).
    float physical = frameBufferValueToPhysicalValue(v);
    float y = physical / pqC; // Normalize for the ST-2084 curve

    float ym = pow(y, m1);
    return exp2(m2 * (log2(c1 + c2 * ym) - log2(1.0f + c3 * ym)));
}

// ICtCp conversion

void rgbToICtCp(vec3 rgb, vec3& ictCp) // Input: linear Rec.2020
{
    float l = (rgb[0] * 1688.0f + rgb[1] * 2146.0f + rgb[2] * 262.0f) / 4096.0f;
    float m = (rgb[0] * 683.0f + rgb[1] * 2951.0f + rgb[2] * 462.0f) / 4096.0f;
    float s = (rgb[0] * 99.0f + rgb[1] * 309.0f + rgb[2] * 3688.0f) / 4096.0f;

    float lPQ = inverseEotfSt2084(l, exponentScaleFactor);
    float mPQ = inverseEotfSt2084(m, exponentScaleFactor);
    float sPQ = inverseEotfSt2084(s, exponentScaleFactor);

    ictCp[0] = (2048.0f * lPQ + 2048.0f * mPQ) / 4096.0f;
    ictCp[1] = (6610.0f * lPQ - 13613.0f * mPQ + 7003.0f * sPQ) / 4096.0f;
    ictCp[2] = (17933.0f * lPQ - 17390.0f * mPQ - 543.0f * sPQ) / 4096.0f;
}

void iCtCpToRgb(vec3 ictCp, vec3& rgb) // Output: linear Rec.2020
{
    float l = ictCp[0] + 0.00860904f * ictCp[1] + 0.11103f * ictCp[2];
    float m = ictCp[0] - 0.00860904f * ictCp[1] - 0.11103f * ictCp[2];
    float s = ictCp[0] + 0.560031f * ictCp[1] - 0.320627f * ictCp[2];

    float lLin = eotfSt2084(l, exponentScaleFactor);
    float mLin = eotfSt2084(m, exponentScaleFactor);
    float sLin = eotfSt2084(s, exponentScaleFactor);

    rgb[0] = max(3.43661f * lLin - 2.50645f * mLin + 0.0698454f * sLin, 0.0f);
    rgb[1] = max(-0.79133f * lLin + 1.9836f * mLin - 0.192271f * sLin, 0.0f);
    rgb[2] = max(-0.0259499f * lLin - 0.0989137f * mLin + 1.12486f * sLin, 0.0f);
}

// Jzazbz conversion

void rgbToJzazbz(vec3 rgb, vec3& jab) // Input: linear Rec.2020
{
    float l = rgb[0] * 0.530004f + rgb[1] * 0.355704f + rgb[2] * 0.086090f;
    float m = rgb[0] * 0.289388f + rgb[1] * 0.525395f + rgb[2] * 0.157481f;
    float s = rgb[0] * 0.091098f + rgb[1] * 0.147588f + rgb[2] * 0.734234f;

    float lPQ = inverseEotfSt2084(l, JZAZBZ_EXPONENT_SCALE_FACTOR);
    float mPQ = inverseEotfSt2084(m, JZAZBZ_EXPONENT_SCALE_FACTOR);
    float sPQ = inverseEotfSt2084(s, JZAZBZ_EXPONENT_SCALE_FACTOR);

    float iz = 0.5f * lPQ + 0.5f * mPQ;

    jab[0] = (0.44f * iz) / (1.0f - 0.56f * iz) - 1.6295499532821566e-11f;
    jab[1] = 3.524000f * lPQ - 4.066708f * mPQ + 0.542708f * sPQ;
    jab[2] = 0.199076f * lPQ + 1.096799f * mPQ - 1.295875f * sPQ;
}

void jzazbzToRgb(vec3 jab, vec3& rgb) // Output: linear Rec.2020
{
    float jz = jab[0] + 1.6295499532821566e-11f;
    float iz = jz / (0.44f + 0.56f * jz);
    float a = jab[1];
    float b = jab[2];

    float l = iz + a * 1.386050432715393e-1f + b * 5.804731615611869e-2f;
    float m = iz + a * -1.386050432715393e-1f + b * -5.804731615611869e-2f;
    float s = iz + a * -9.601924202631895e-2f + b * -8.118918960560390e-1f;

    float lLin = eotfSt2084(l, JZAZBZ_EXPONENT_SCALE_FACTOR);
    float mLin = eotfSt2084(m, JZAZBZ_EXPONENT_SCALE_FACTOR);
    float sLin = eotfSt2084(s, JZAZBZ_EXPONENT_SCALE_FACTOR);

    rgb[0] = lLin * 2.990669f + mLin * -2.049742f + sLin * 0.088977f;
    rgb[1] = lLin * -1.634525f + mLin * 3.145627f + sLin * -0.483037f;
    rgb[2] = lLin * -0.042505f + mLin * -0.377983f + sLin * 1.448019f;
}

// Unified color space (UCS): ICtCp or Jzazbz

#if TONE_MAPPING_UCS == TONE_MAPPING_UCS_ICTCP
void rgbToUcs(vec3 rgb, vec3& ucs) { rgbToICtCp(rgb, ucs); }
void ucsToRgb(vec3 ucs, vec3& rgb) { iCtCpToRgb(ucs, rgb); }
#elif TONE_MAPPING_UCS == TONE_MAPPING_UCS_JZAZBZ
void rgbToUcs(vec3 rgb, vec3& ucs) { rgbToJzazbz(rgb, ucs); }
void ucsToRgb(vec3 ucs, vec3& rgb) { jzazbzToRgb(ucs, rgb); }
#else
#error                                                                                                                 \
    "Unsupported TONE_MAPPING_UCS value. Please define TONE_MAPPING_UCS as either TONE_MAPPING_UCS_ICTCP or TONE_MAPPING_UCS_JZAZBZ."
#endif

// "GT Tone Mapping" curve with convergent shoulder

struct GTToneMappingCurveV2 {
    float peakIntensity_;
    float alpha_;
    float midPoint_;
    float linearSection_;
    float toeStrength_;
    float kA_, kB_, kC_;
};

void initializeCurve(float monitorIntensity, float alpha, float grayPoint, float linearSection, float toeStrength,
    GTToneMappingCurveV2& Curve) {
    Curve.peakIntensity_ = monitorIntensity;
    Curve.alpha_ = alpha;
    Curve.midPoint_ = grayPoint;
    Curve.linearSection_ = linearSection;
    Curve.toeStrength_ = toeStrength;

    // Pre-compute constants for the shoulder region.
    float k = (Curve.linearSection_ - 1.0f) / (Curve.alpha_ - 1.0f);
    Curve.kA_ = Curve.peakIntensity_ * Curve.linearSection_ + Curve.peakIntensity_ * k;
    Curve.kB_ = -Curve.peakIntensity_ * k * exp(Curve.linearSection_ / k);
    Curve.kC_ = -1.0f / (k * Curve.peakIntensity_);
}

float evaluateCurve(float x, GTToneMappingCurveV2 Curve) {
    if (x < 0.0f) {
        return 0.0f;
    }

    float weightLinear = smoothStep(x, 0.0f, Curve.midPoint_);
    float weightToe = 1.0f - weightLinear;

    // Shoulder mapping for highlights.
    float shoulder = Curve.kA_ + Curve.kB_ * exp(x * Curve.kC_);

    if (x < Curve.linearSection_ * Curve.peakIntensity_) {
        float toeMapped = Curve.midPoint_ * pow(x / Curve.midPoint_, Curve.toeStrength_);
        return weightToe * toeMapped + weightLinear * x;
    } else {
        return shoulder;
    }
}

// GT7ToneMapping

struct GT7ToneMapping {
    float sdrCorrectionFactor_;

    float framebufferLuminanceTarget_;
    float framebufferLuminanceTargetUcs_; // Target luminance in UCS space
    GTToneMappingCurveV2 curve_;

    float blendRatio_;
    float fadeStart_;
    float fadeEnd_;
};

void initializeParameters(float physicalTargetLuminance, GT7ToneMapping& TM) {
    TM.framebufferLuminanceTarget_ = physicalValueToFrameBufferValue(physicalTargetLuminance);

    // Initialize the curve (slightly different parameters from GT Sport).
    initializeCurve(TM.framebufferLuminanceTarget_, 0.25f, 0.538f, 0.444f, 1.280f, TM.curve_);

    // Default parameters.
    TM.blendRatio_ = 0.6f;
    TM.fadeStart_ = 0.98f;
    TM.fadeEnd_ = 1.16f;

    vec3 ucs;
    vec3 rgb = vec3(TM.framebufferLuminanceTarget_, TM.framebufferLuminanceTarget_, TM.framebufferLuminanceTarget_);
    rgbToUcs(rgb, ucs);
    TM.framebufferLuminanceTargetUcs_ = ucs[0]; // Use the first UCS component (I or Jz) as luminance
}

void initializeAsHDR(float physicalTargetLuminance, GT7ToneMapping& TM) {
    TM.sdrCorrectionFactor_ = 1.0f;
    initializeParameters(physicalTargetLuminance, TM);
}

void initializeAsSDR(GT7ToneMapping& TM) {

    TM.sdrCorrectionFactor_ = 1.0f / physicalValueToFrameBufferValue(GRAN_TURISMO_SDR_PAPER_WHITE);
    initializeParameters(GRAN_TURISMO_SDR_PAPER_WHITE, TM);
}

void applyToneMapping(vec3& rgb, GT7ToneMapping TM) {
    // Convert to UCS to separate luminance and chroma.
    vec3 ucs;
    rgbToUcs(rgb, ucs);

    // Per-channel tone mapping ("skewed" color).
    vec3 skewedRgb =
        vec3(evaluateCurve(rgb[0], TM.curve_), evaluateCurve(rgb[1], TM.curve_), evaluateCurve(rgb[2], TM.curve_));

    vec3 skewedUcs;
    rgbToUcs(skewedRgb, skewedUcs);

    float chromaScale = chromaCurve(ucs[0] / TM.framebufferLuminanceTargetUcs_, TM.fadeStart_, TM.fadeEnd_);

    vec3 scaledUcs = vec3(skewedUcs[0], // Luminance from skewed color
        ucs[1] * chromaScale,           // Scaled chroma components
        ucs[2] * chromaScale);

    // Convert back to RGB.
    vec3 scaledRgb;
    ucsToRgb(scaledUcs, scaledRgb);

    // Final blend between per-channel and UCS-scaled results.
    for (int i = 0; i < 3; ++i) {
        float blended = (1.0f - TM.blendRatio_) * skewedRgb[i] + TM.blendRatio_ * scaledRgb[i];
        // When using SDR, apply the correction factor.
        // When using HDR, sdrCorrectionFactor_ is 1.0f, so it has no effect.
        rgb[i] = TM.sdrCorrectionFactor_ * min(blended, TM.framebufferLuminanceTarget_);
    }
}


static BasicMaterial g_default_mat = BasicMaterial({ 1.0f, 1.0f, 1.0f }, { 1, 1, 1 }, 64.0f);
static TriangleDebugMaterial g_debug_mat = TriangleDebugMaterial();
static GridMaterial g_grid_mat = GridMaterial();
static std::vector<PointLight> g_point_lights;

PathTracer::PathTracer() {
    m_scene = new Scene();
    LoadScene();
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

    m_accumulation_count = 0;
    m_accumulator.clear();
}

void PathTracer::OnUpdate(float frame_time) {
    static float accum = 0.0f;
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
    float jitter_x = RandomFloatAdv(seed) - 0.5f;
    float jitter_y = RandomFloatAdv(seed) - 0.5f;

    // 2. Add to pixel coordinate FIRST
    // (x + 0.5) centers us in the pixel.
    // (+ jitter) moves us randomly within it.
    float px = (float)x + 0.5f + jitter_x;
    float py = (float)y + 0.5f + jitter_y;

    // 3. Convert to NDC (0..1 -> -1..1)
    // Note: We use the jittered 'px' and 'py' here
    glm::vec2 ndc;
    ndc.x = (px * m_inv_width_height.x) * 2.0f - 1.0f;
    ndc.y = (py * m_inv_width_height.y) * 2.0f - 1.0f;

    // 4. Apply Aspect Ratio & Flip (Now safe to do!)
    ndc.x *= m_ar; // usually we scale X by AR, but your code scales Y. Adapt as needed.
    // ndc.y /= m_ar; // (If you stick to your previous scaling method)
    ndc.y = -ndc.y; // Flip Y

    // debug NDC
    // return DOOB_WRITE_PIXEL_F32(ndc.x * 0.5f + 0.5f, ndc.y * 0.5f + 0.5f, 0.0f, 1.0f);

    const Ray ray = m_camera.GetRay(ndc);

    Intersection intersection;
    DrawableActor actor;

    glm::vec3 color = {};
    if (!this->IntersectFirstActor(ray, &intersection, &actor)) {
        // No intersection, sky colour
        color = this->SampleSky(ray.direction);
    } else {
        const glm::vec3 P = intersection.t * ray.direction + ray.origin;

        const LightInput light_input{
            .eye = ray.origin,
            .P = P,
            .V = glm::normalize(ray.origin - P),
            .N = intersection.flat_normal,
        };
        color = this->ShadeActor(actor, light_input, intersection, seed);
    }
    glm::dvec3& summed = m_accumulator[static_cast<size_t>(x) + y * m_width];
    if (m_parameters.b_accumulate) {
        summed += static_cast<glm::dvec3>(color);
    } else {
        summed = static_cast<glm::dvec3>(color);
    }

    glm::vec3 color_accumulated = static_cast<glm::vec3>(summed / static_cast<double>(m_accumulation_count));
    if (m_parameters.b_gt7_tonemapper) {
        GT7ToneMapping TM;
        initializeAsSDR(TM);
        applyToneMapping(color_accumulated, TM);
    } else {
        color_accumulated = 1.0f - exp(-color_accumulated);
    }

    return DOOB_WRITE_PIXEL_F32(color_accumulated.r, color_accumulated.g, color_accumulated.b, 1.0f);
}
void PathTracer::RebuildAccelerationStructures() {}
void PathTracer::LoadScene() {

    static Plane plane1 = Plane({ 0, 1, 0 }, { 0, -1, 0 });
    m_plane_actor = m_scene->NewDrawableActor(&plane1, &g_grid_mat);


    // static std::vector<std::pair<Sphere, BasicMaterial>> objects;
    // objects.clear();
    // objects.reserve(100);

    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_real_distribution<float> posDist(-10.0f, 10.0f); // Range -10 to 10
    // std::uniform_real_distribution<float> radDist(0.5f, 1.5f);    // Radius 0.5 to 1.5
    // std::uniform_real_distribution<float> colDist(0.0f, 1.0f);    // Colour 0.0 to 1.0

    // for (int i = 0; i < 100; ++i) {
    //     float x = posDist(gen);
    //     float y = posDist(gen);
    //     float z = posDist(gen) + 5.0f; // Offset Z so they are in front of camera
    //     float radius = radDist(gen);

    //    float r = colDist(gen);
    //    float g = colDist(gen);
    //    float b = colDist(gen);

    //    objects.emplace_back(Sphere({ x, y, z }, radius), BasicMaterial({ r, g, b }, { 1, 1, 1 }, 64.f));
    //}
    //
    // for (auto& [shape, material] : objects) {
    //    ActorId id = m_scene->NewDrawableActor(&shape, &material);
    //}

    static Sphere sphere1 = Sphere({ 0.0f, 0.0f, 5.0f }, 1.0f);
    ActorId sphere_actor = m_scene->NewDrawableActor(&sphere1, &g_default_mat);

    static Triangle triangle1 = Triangle({ 0, 0, 3 }, { 0, 3, 3 }, { 1, 1, 4 });
    // ActorId triangle_actor = m_scene->NewDrawableActor(&triangle1, &g_debug_mat);

    std::vector<glm::vec3> light_colors{ { 1.f, 1.f, 1.f }, { 1.f, .1f, .1f }, { .1f, .1f, 1.f }, { .1f, 1.f, .1f },
        { 1.f, 1.f, .1f }, { .1f, 1.f, 1.f } };
    g_point_lights.clear();
    g_point_lights.reserve(light_colors.size());
    for (int i = 0; i < light_colors.size(); ++i) {
        g_point_lights.push_back(PointLight({ 0, 10, 0 }, light_colors[i] * 10.0f));
        m_light_actor = m_scene->NewLightActor(&g_point_lights.back());
    }

    static AreaLight area_light1 = AreaLight({ 0, 4.0f, 8 }, { 1, 2 }, { 60, 60, 60 });
    ActorId light_actor1 = m_scene->NewLightActor(&area_light1);

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
void PathTracer::Cleanup() {}
bool PathTracer::IntersectFirstActor(const Ray& ray, Intersection* out_intersection, DrawableActor* out_actor) const {
    Intersection intersection{ .t = INFINITY };
    DrawableActor current_actor;

    for (const auto& actor : m_drawable_actors) {
        Intersection curr_intersection;
        if (!actor.shape->Intersect(ray, &curr_intersection)) {
            continue;
        }
        if (curr_intersection.t < intersection.t) {
            intersection = curr_intersection;
            current_actor = actor;
        }
    }
    if (glm::isinf(intersection.t)) {
        return false;
    }
    if (out_actor) {
        *out_actor = current_actor;
    }
    if (out_intersection) {
        *out_intersection = intersection;
    }
    return true;
}

DOOB_NODISCARD bool PathTracer::IntersectAnyActor(const Ray& ray) const {
    for (const auto& actor : m_drawable_actors) {
        if (actor.shape->Intersect(ray, nullptr)) {
            return true;
        }
    }
    return false;
}

glm::vec3 PathTracer::ShadeActor(const DrawableActor& actor, const LightInput& shading_input,
    const Intersection& intersection, uint32_t& seed) const {
    DrawableActor curr_actor = actor;
    LightInput curr_shading_input = shading_input;
    Intersection curr_intersection = intersection;
    glm::vec3 total = {};
    glm::vec3 diffuse_absorption = { 1, 1, 1 };
    glm::vec3 specular_absorption = { 1, 1, 1 };
    for (int b = 0; b <= m_parameters.max_light_bounces; ++b) {
        bool b_stop = false;

        glm::vec3 diffuse = {};
        glm::vec3 specular = {};
        Fragment fragment = curr_actor.shape->SampleFragment(curr_intersection);

        MaterialOutput material = curr_actor.material->Evaluate(fragment);
        curr_shading_input.N = material.world_normal;
        // DIRECT LIGHTING
        {
            for (const auto& [light_obj] : m_light_actors) {
                LightOutput result =
                    light_obj->Evaluate(curr_shading_input, { .specular_power = material.specular_power }, seed,
                        {
                            .userdata = reinterpret_cast<const void*>(this),
                            .fn_shadow_check =
                                [](const Ray& ray, const void* userdata) {
                                    return !reinterpret_cast<const PathTracer*>(userdata)->IntersectAnyActor(ray);
                                },
                        });
                diffuse += result.diffuse;
                specular += result.specular;
            }
        }

        float fresnel = glm::max(glm::dot(curr_shading_input.V, curr_shading_input.N), 0.0f);
        float f2 = fresnel * fresnel;
        float f4 = f2 * f2;
        float f5 = f4 * fresnel;
        float kD = fresnel;

        // SPECULAR LIGHTING (Reflections)
        if (b < m_parameters.max_light_bounces) {
            glm::vec3 R = glm::reflect(-curr_shading_input.V, curr_shading_input.N);
            curr_shading_input.eye = curr_shading_input.P;

            Ray ray{ .origin = curr_shading_input.P, .direction = R };
            // offset ray to avoid bias
            ray.origin += glm::sign(curr_shading_input.N) * glm::abs(ray.origin * 0.0000002f);
            DrawableActor actor;
            if (!this->IntersectFirstActor(ray, &curr_intersection, &actor)) {
                // reached sky
                specular += this->SampleSky(R);
                b_stop = true;
            } else {
                curr_shading_input.P = curr_intersection.position;
                curr_shading_input.V = glm::normalize(ray.origin - curr_intersection.position);
                curr_actor = actor;
            }
        }

        total += kD * diffuse_absorption * material.albedo_color * diffuse +
                 (1.0f - kD) * specular_absorption * material.specular_color * specular;

        diffuse_absorption *= material.albedo_color;
        specular_absorption *= material.specular_color;

        if (b_stop) {
            break;
        }
    }
    return total;
}
DOOB_NODISCARD glm::vec3 PathTracer::SampleSky(const glm::vec3& R) const {
    // TODO skybox
    static glm::vec3 ambient_color = { 0.51f, 0.53f, 0.54f };
    return ambient_color;
}
} // namespace devs_out_of_bounds