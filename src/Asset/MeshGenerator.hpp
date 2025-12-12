#pragma once

#include <filesystem>
#include <memory>
#include <numbers>
#include <optional>
#include <src/Asset/Common.hpp>
#include <string>
#include <variant>
#include <vector>


namespace devs_out_of_bounds {
struct IModelLoader {
public:
    IModelLoader() = default;
    virtual ~IModelLoader() = default;

    virtual ModelData Load(const std::filesystem::path& filepath) = 0;
    virtual ImageData ReadImageData(const ModelData& modelData, const Image& image) = 0;
    virtual MeshData ReadMeshData(const ModelData& modelData, const Mesh& mesh) = 0;
};
} // namespace devs_out_of_bounds
