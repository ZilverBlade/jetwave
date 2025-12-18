#pragma once
#include <src/Graphics/IMaterial.hpp>
#include <src/Graphics/ISamplerState.hpp>
#include <src/Graphics/ITextureView.hpp>

#include <src/Graphics/BxDFs/GgxMicrofacetBrdf.hpp>
#include <src/Graphics/BxDFs/LambertBrdf.hpp>
#include <src/Graphics/BxDFs/PassthroughBtdf.hpp>

namespace devs_out_of_bounds {
namespace material {
    class GltfMaterial : public IMaterial {
    public:
        enum struct BlendMode : uint8_t {
            Opaque,
            Mask,
            Blend,
        };

        void Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const override {
            using namespace glm;
            using glm::vec3;
            if (!out_bsdf) {
                return;
            }
            if (!b_double_sided && !input.b_front_face) {
                out_bsdf->Add<bxdf::PassthroughBtdf>(glm::vec3(1, 1, 1), 0.0f);
                return;
            }

            vec4 base_color = base_color_factor;
            if (sampler_state && base_color_texture) {
                vec4 col = sampler_state->Sample(base_color_texture, input.uv);

                // FIXME: srgb
                col.r = pow(col.r, 2.2f);
                col.g = pow(col.g, 2.2f);
                col.b = pow(col.b, 2.2f);

                base_color *= col;
            }
            if (blend_mode == BlendMode::Mask) {
                if (base_color.a < alpha_cutoff) {
                    out_bsdf->Add<bxdf::PassthroughBtdf>(glm::vec3(1, 1, 1), 0.0f);
                    return;
                }
            } else if (blend_mode == BlendMode::Blend) {
                out_bsdf->Add<bxdf::PassthroughBtdf>(glm::vec3(1, 1, 1), base_color.a);
            }

            glm::vec3 emission = emissive_factor; 

            glm::vec3 world_normal = input.normal;
            if (!input.b_front_face) {
                world_normal = -world_normal;
            }
            float rough = roughness_factor, metal = metallic_factor;
            if (sampler_state) {
                if (metallic_roughness_texture) {
                    vec4 res = sampler_state->Sample(metallic_roughness_texture, input.uv);
                    metal *= res.b;
                    rough *= res.g;
                    rough = max(rough, 0.02f);
                }
                if (emissive_texture) {
                    vec3 res = sampler_state->Sample(emissive_texture, input.uv);
                    emission *= pow(res, vec3(2.2f)); // FIXME: srgb
                }
                if (normal_texture && glm::dot(input.tangent, input.tangent) > std::numeric_limits<float>::epsilon()) {
                    vec3 nor = sampler_state->Sample(normal_texture, input.uv) * 2.0f - 1.0f;

                    // Check for valid tangent to avoid NaNs
                    if (glm::dot(input.tangent, input.tangent) > 1e-6f) {
                        vec3 T = normalize(input.tangent);
                        vec3 N = input.flat_normal; 

                        T = normalize(T - N * dot(N, T));

                        vec3 B = cross(N, T);

                        mat3 TBN = mat3(T, B, N);

                        vec3 map_normal = normalize(vec3(vec2(nor) * normal_strength, nor.z));
                        world_normal = normalize(TBN * map_normal);
                    }
                } else {
                    world_normal = normalize(input.normal);
                }
            }
            // FIXME: srgb
            *out_emission = emission * emissive_intensity;

            out_bsdf->Add<bxdf::LambertBrdf>(vec3(base_color) * (1.0f - metal), world_normal);
            out_bsdf->Add<bxdf::GgxMicrofacetBrdf>(mix(glm::vec3(0.04f), vec3(base_color), metal), rough, world_normal);
        }

        ISamplerState* sampler_state = {};

        ITextureView* base_color_texture = {};
        glm::vec4 base_color_factor = { 1, 1, 1, 1 };

        ITextureView* metallic_roughness_texture = {};
        float metallic_factor = 1.0f;
        float roughness_factor = 1.0f;

        ITextureView* normal_texture = {};
        float normal_strength = 1.0f;

        ITextureView* emissive_texture = {};
        glm::vec3 emissive_factor = glm::vec3{ 0.0f };
        float emissive_intensity = 100000.0f;

        BlendMode blend_mode = BlendMode::Opaque;
        float alpha_cutoff = 0.5f;

        bool b_double_sided = false;
    };
} // namespace material
} // namespace devs_out_of_bounds