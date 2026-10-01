#pragma once
#include <cstdint>
#include <unordered_map>
#include <limits>
#include <assert.h>
#include <string>
#include <optional>
#include <vector>

struct GPUTexture
{
};

enum class GPUImageFormat
{
    Color,
    Depth
};
enum class GPUTextureUsage : uint32_t
{
    RenderTarget = 1 << 0,
    ShaderRead = 1 << 1,
    Depth = 1 << 2
};

struct GPUTextureDesc
{
    struct Size
    {
        uint32_t width = 1;
        uint32_t height = 1;
    } size;
    GPUImageFormat format;
    GPUTextureUsage usage;
    uint32_t mipCount = 1;
};
struct GPUBuffer
{
};

enum class GPUBufferUsage : uint32_t
{
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2
};

enum class GPUBufferStorageMode
{
    GPU,
    Shared,
    Dynamic
};
struct GPUBufferDesc
{
    size_t size;
    GPUBufferUsage usage;
    GPUBufferStorageMode storageMode;
};

enum class ResourceLifetimeType
{
    Persistent,
    Transient
};

enum class ResourceInstanceType
{
    Single,
    PerFrame,
    Temporal
};
using HandleId = uint32_t;
constexpr uint32_t INVALID_HANDLE_ID = std::numeric_limits<HandleId>::max();
template <typename T>
struct Handle
{
    HandleId id = INVALID_HANDLE_ID;
    uint32_t generation = 0;

    bool isValid() const
    {
        return id != INVALID_HANDLE_ID;
    }
};

struct TextureTag
{
};
struct BufferTag
{
};

using TextureHandle = Handle<TextureTag>;
using BufferHandle = Handle<BufferTag>;
