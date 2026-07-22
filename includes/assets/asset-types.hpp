#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
struct Texture
{
    int width, height, channels;
    std::vector<uint8_t> pixels;
};
struct Shader
{
    std::vector<uint8_t> bytecode;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal = {0.0f, 0.0f, 1.0f};
    glm::vec2 uv = {0.0f, 0.0f};
    glm::vec4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};
};
struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
