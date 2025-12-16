#pragma once
#include <src/Graphics/IMaterial.hpp>
#include <src/Graphics/ISamplerState.hpp>
#include <src/Graphics/ITextureView.hpp>

#include <src/Graphics/BxDFs/GgxMicrofacetBrdf.hpp>
#include <src/Graphics/BxDFs/LambertBrdf.hpp>
#include <src/Graphics/BxDFs/PassthroughBtdf.hpp>

namespace devs_out_of_bounds {
namespace material {
    enum struct BlendMode : uint8_t {
        Opaque,
        Mask,
        Blend,
    };
    class GltfMaterial : public IMaterial {
    public:
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
                base_color *= sampler_state->Sample(base_color_texture, input.uv);
            }
            if (blend_mode == BlendMode::Mask) {
                if (base_color.a < alpha_cutoff) {
                    out_bsdf->Add<bxdf::PassthroughBtdf>(glm::vec3(1, 1, 1), 0.0f);
                    return;
                }
            } else if (blend_mode == BlendMode::Blend) {
                out_bsdf->Add<bxdf::PassthroughBtdf>(glm::vec3(1, 1, 1), base_color.a);
            }
            *out_emission = emissive_factor;

            glm::vec3 world_normal = input.normal;
            float rough = roughness_factor, metal = metallic_factor;
            if (sampler_state) {
                if (metallic_roughness_texture) {
                    vec2 res = sampler_state->Sample(metallic_roughness_texture, input.uv);
                    metal *= res.r;
                    rough *= res.g;
                }
                if (emissive_texture) {
                    vec3 res = sampler_state->Sample(emissive_texture, input.uv);
                    *out_emission *= res;
                }
                if (normal_texture) {
                    vec3 nor = sampler_state->Sample(emissive_texture, input.uv) * 2.0f - 1.0f;
                    nor.x *= normal_strength;
                    nor.y *= normal_strength;

                    const vec3 bitangent = cross(input.tangent, input.normal);

                    mat3 TBN = { input.tangent, bitangent, input.normal };

                    world_normal = normalize(TBN * nor);
                } else {
                    world_normal = input.normal;
                }
            }

            out_bsdf->Add<bxdf::LambertBrdf>(glm::vec3(base_color) * (1.0f - metallic_factor), world_normal);
            out_bsdf->Add<bxdf::GgxMicrofacetBrdf>(
                glm::mix(glm::vec3(0.04f), glm::vec3(base_color), metallic_factor), roughness_factor, world_normal);
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

        BlendMode blend_mode = BlendMode::Opaque;
        float alpha_cutoff = 0.5f;

        bool b_double_sided = false;
    };
} // namespace material
} // namespace devs_out_of_bounds