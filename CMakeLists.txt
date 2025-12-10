cmake_minimum_required(VERSION 3.10)
project(xwarp)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

set(STB_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stbimage")

# add_subdirectory(vendor)

set(SRC_MAIN_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE SRC_MAIN_CPP_SOURCE
      "${SRC_MAIN_CPP}/*.hpp"
      "${SRC_MAIN_CPP}/*.cpp")


if (WIN32)
    add_executable(xwave WIN32 
    ${SRC_MAIN_CPP_SOURCE}
    )
else()
    add_executable(xwave 
    ${SRC_MAIN_CPP_SOURCE}
    )
endif()

# target_link_libraries(xwave PRIVATE
#     glm::glm-header-only
#     fmt::fmt
#     sdl
# )


# target_include_directories(xwave PRIVATE
#     ${STB_IMAGE_DIR}
# )
target_include_directories(xwave
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)













