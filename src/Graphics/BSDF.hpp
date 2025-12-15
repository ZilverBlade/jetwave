#pragma once

#include <src/Graphics/IBxDF.hpp>
#include <type_traits>

namespace devs_out_of_bounds {

class BSDF {
public:
    BSDF() {
        m_bxdfs.reserve(32);
        m_memory.resize(512);
    }
    ~BSDF() { Reset(); }

    void Reset() {
        for (auto it = m_bxdfs.rbegin(); it != m_bxdfs.rend(); ++it) {
            (*it)->~IBxDF();
        }
        m_bxdf_mem_ptr = 0;
        m_bxdfs.clear();
        m_bxdf_sizes.clear();
        m_inv_bxdfs = 1.f;
    }
    template <BxDFConcept TBxDF, typename... TArgs>
    void Add(TArgs&&... args) {
        // optimal packing
        if (m_bxdf_mem_ptr + sizeof(TBxDF) > m_memory.size()) {
            m_memory.resize(m_memory.size() * 2 + sizeof(TBxDF));
            size_t accum = 0;
            for (int i = 0; i < m_bxdfs.size(); ++i) {
                m_bxdfs[i] = reinterpret_cast<IBxDF*>(m_memory.data() + accum);
                accum += m_bxdf_sizes[i];
            }
        }
        m_bxdf_sizes.push_back(sizeof(TBxDF));
        TBxDF* bxdf = reinterpret_cast<TBxDF*>(m_memory.data() + m_bxdf_mem_ptr);
        new (bxdf) TBxDF(std::forward<TArgs>(args)...);
        m_bxdfs.push_back(bxdf);
        m_bxdf_mem_ptr += sizeof(TBxDF);
        m_inv_bxdfs = 1.f / static_cast<float>(m_bxdfs.size());
    }
    bool HasBxDF() const { return m_bxdfs.size() > 0; }

    glm::vec3 Evaluate(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const {
        glm::vec3 result(0.0f);
        for (const auto* lobe : m_bxdfs) {
            result += lobe->Evaluate(wo, wm, wi);
        }
        return result;
    }

    glm::vec3 Sample_Evaluate(const glm::vec3& wo, glm::vec3& wi, uint32_t& seed, float& pdf) {
        if (m_bxdfs.empty())
            return glm::vec3(0.0f);

        float r = RandomFloatAdv<UniformDistribution>(seed);
        int comp = std::min((int)(r * m_bxdfs.size()), (int)m_bxdfs.size() - 1);
        IBxDF* chosen_lobe = m_bxdfs[comp];

        BxDFType sampled_type = chosen_lobe->Type();
        wi = chosen_lobe->NextSample(wo, seed);
        float VdL = glm::abs(glm::dot(wi, wo)); // ensure wi is used
        glm::vec3 wm = VdL > (1 - std::numeric_limits<float>::epsilon()) ? wo : glm::normalize(wi + wo);
        pdf = chosen_lobe->Pdf(wo, wm, wi) * m_inv_bxdfs;
        return chosen_lobe->Evaluate(wo, wm, wi);
    }

private:
    std::vector<IBxDF*> m_bxdfs = {};
    std::vector<size_t> m_bxdf_sizes = {};
    std::vector<char> m_memory = {};
    size_t m_bxdf_mem_ptr = 0;

    float m_inv_bxdfs = 1.0f;
};
} // namespace devs_out_of_bounds