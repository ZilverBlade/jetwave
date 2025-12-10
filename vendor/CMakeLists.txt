add_subdirectory(fmt)

# glm
set(GLM_ENABLE_CXX_20 ON)
set(GLM_ENABLE_SIMD_AVX2 ON)
add_subdirectory(glm)

set(SDL_SHARED ON)
set(SDL_AVX2 ON)
add_subdirectory(SDL)
