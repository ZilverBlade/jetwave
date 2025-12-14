#pragma once
#include <src/Graphics/IMaterial.hpp>
#include <src/Graphics/ISamplerState.hpp>
#include <src/Graphics/ITextureView.hpp>

namespace devs_out_of_bounds {
namespace material {
    enum struct BlendMode : uint8_t {
        Opaque,
        Mask,
        Blend,
    };
    class GltfMaterial : public IMaterial {
    public:
        DOOB_NODISCARD MaterialOutput Evaluate(const Fragment& input) const override {
            using namespace glm;
            using glm::vec3;

            MaterialOutput output;
            vec4 base_color = base_color_factor;
            if (sampler_state && base_color_texture) {
                base_color *= sampler_state->Sample(base_color_texture, input.uv);
            }
            if (blend_mode == BlendMode::Blend) {
                output.opacity = base_color.a;
            } else {
                output.opacity = 1.0f;
            }
            output.albedo_color = base_color_factor;
            output.emission_color = emissive_factor;

            float rough = roughness_factor, metal = metallic_factor;
            if (sampler_state) {
                if (metallic_roughness_texture) {
                    vec2 res = sampler_state->Sample(metallic_roughness_texture, input.uv);
                    metal *= res.r;
                    rough *= res.g;
                }
                if (emissive_texture) {
                    vec3 res = sampler_state->Sample(emissive_texture, input.uv);
                    output.emission_color *= res;
                }
                if (normal_texture) {
                    vec3 nor = sampler_state->Sample(emissive_texture, input.uv) * 2.0f - 1.0f;
                    nor.x *= normal_strength;
                    nor.y *= normal_strength;

                    const vec3 bitangent = cross(input.tangent, input.normal);

                    mat3 TBN = { input.tangent, bitangent, input.normal };

                    output.world_normal = normalize(TBN * nor);
                } else {
                    output.world_normal = input.normal;
                }
            }

            // Mimic PBR effects here
            // Convert Roughness to Glossiness (Perceptual)
            float perceptual_roughness = rough;
            // Often squared to make the slider feel more linear to the eye (Disney model)
            float alpha = perceptual_roughness * perceptual_roughness;

            // Calculate Specular Power (Shininess)
            // A common approximation matching GGX distribution is: 2 / alpha^2 - 2
            // But for simple Phong, a logarithmic scale usually looks best:
            // This maps 0 roughness -> High Power, 1 roughness -> Low Power
            float shininess = exp2(11.0f * (1.0f - perceptual_roughness)); // Range ~1 to ~2048

            output.specular_power = clamp(shininess, 1.0f, 2048.0f);
            output.specular_color = mix({ 1, 1, 1 }, output.albedo_color, metallic_factor);
            output.albedo_color *= (1.0f - metallic_factor);

            float energy_conservation = (output.specular_power + 8.0f) / (8.0f * pi<float>());
            output.specular_color *= energy_conservation;

            return output;
        }
        DOOB_NODISCARD bool EvaluateDiscard(const Fragment& input) const override {
            using namespace glm;
            using glm::vec3;

            if (!b_double_sided && !input.b_front_face) {
                return true;
            }
            MaterialOutput output;
            vec4 base_color = base_color_factor;
            if (sampler_state && base_color_texture) {
                base_color *= sampler_state->Sample(base_color_texture, input.uv);
            }
            if (blend_mode == BlendMode::Blend) {
                return true;
            } else {
                if (blend_mode == BlendMode::Mask && base_color.a < alpha_cutoff) {
                    return true;
                }
            }
            return false;
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
        glm::vec3 emissive_factor = vec3{ 0.0f };

        BlendMode blend_mode = BlendMode::Opaque;
        float alpha_cutoff = 0.5f;

        bool b_double_sided = false;
    };
} // namespace material
} // namespace devs_out_of_bounds