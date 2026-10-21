#pragma once
#include <cstdint>
#include <unordered_map>
#include <limits>
#include <assert.h>
#include <string>
#include <optional>
#include <vector>
#include <variant>

enum class ResourceState
{
    ShaderWrite,
    ShaderRead,
    Indirect,
    Undefined
};

struct TextureState
{

    TextureState(uint mipCount = 1, uint layers = 1) : perSub(mipCount * layers, ResourceState::Undefined)
    {
    }
    bool uniform = true;
    ResourceState all = ResourceState::Undefined;
    std::vector<ResourceState> perSub;

    ResourceState get(uint32_t sub) const { return uniform ? all : perSub[sub]; }
};
enum class ResourceSizeType
{
    Screen,
    Absolute,
    Tile
};
struct ResourceSize
{

    enum class Type
    {
        Absolute,
        ScreenRelative,
        TileRelative
    } type = Type::Absolute;
    uint32_t width = 1, height = 1;
    float scale = 1.0f;

    size_t bytesPerTile = 1;
    size_t tileSize = 16;

    uint32_t calculateTextureWidth(const uint32_t screenWidth);
    uint32_t calculateTextureHeight(const uint32_t screenHeight);
    size_t calculateBufferSize(const uint32_t screenWidth, const uint32_t screenHeight);
    static ResourceSize bufferAbsolute(size_t bytes);
    static ResourceSize bufferScreenRelative(size_t bytesPerPixel, float scale = 1.0f);
    static ResourceSize bufferTileRelative(uint32_t tileSize, size_t bytesPerTile);

    static ResourceSize textureAbsolute(uint32_t w, uint32_t h);
    static ResourceSize textureScreenRelative(float scale = 1.0f);
};

struct Extent2D
{
    uint32_t width, height;

    const bool operator==(const Extent2D &other) const
    {
        return width == other.width && height == other.height;
    };
    const bool operator!=(const Extent2D &other) const
    {
        return !(*this == other);
    };
};
struct GPUTexture
{

    Extent2D extent;
    ResourceState state = ResourceState::Undefined;
    // Make use of a list for mip x layer but this would be done in the rhi part
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
    ResourceSize size;
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
    ResourceSize size;
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
    bool operator==(const Handle<T> &o) const
    {
        return o.id == id && o.generation == generation;
    }
};

template <typename T>
struct HandleHash
{
    size_t operator()(const Handle<T> &handle) const
    {
        return std::hash<uint64_t>()(handle.id);
    };
};

struct TextureTag
{
};
struct BufferTag
{
};

using TextureHandle = Handle<TextureTag>;
using BufferHandle = Handle<BufferTag>;
using TextureHandleHash = HandleHash<TextureTag>;

struct ResourceVersion
{
    std::optional<int> producer{
        std::nullopt};
    std::vector<int> consumers;
};

template <typename T>
struct LogicalResource
{

    std::string name;
    T physicalHandle;
    std::vector<ResourceVersion> versions;
    bool history = false;
};

using LogicalTexture = LogicalResource<TextureHandle>;
using LogicalBuffer = LogicalResource<BufferHandle>;

using LogicalTextureHandle = Handle<LogicalTexture>;
using LogicalBufferHandle = Handle<LogicalBuffer>;

template <typename T>
struct TemporalLogicalResource
{
    T current;
    T previous;
};

using TemporalLogicalTexture = TemporalLogicalResource<LogicalTextureHandle>;
using TemporalLogicalBuffer = TemporalLogicalResource<LogicalBufferHandle>;

template <typename T>
struct ResourceAccess
{
    Handle<T> resource;
    ResourceState state;
    bool history = false;
};

using TextureAccess = ResourceAccess<TextureTag>;
using BufferAccess = ResourceAccess<BufferTag>;

struct TextureBarrier
{
    TextureAccess access;
};

struct BufferBarrier
{
    BufferAccess access;
};

struct CompiledRenderGraph
{
    struct Step
    {
        std::vector<TextureAccess> textureBarriers;
        std::vector<BufferAccess> bufferBarriers;
        int passIdx;
    };
    std::vector<Step> steps;
};

struct ResourceLifetimeV2
{
    std::string name;
    int lastUse = -1;
    int createdAt = -1;
};

using PhysicalTextureLifetime = std::unordered_map<TextureHandle, ResourceLifetimeV2, TextureHandleHash>;
struct HeapMemoryRequirement
{
    size_t size;
    size_t memorytypeIndex;
};
using HeapId = uint32_t;
struct AliasGroup
{
    HeapId id;
    HeapMemoryRequirement requirement;
    std::vector<TextureHandle> textures;
};

struct AliasPlan
{
    std::unordered_map<HeapId, AliasGroup> groups;
    std::unordered_map<TextureHandle, HeapId, TextureHandleHash> textureToHeap;
    std::unordered_map<TextureHandle, Extent2D, TextureHandleHash> textureExtents;
};
