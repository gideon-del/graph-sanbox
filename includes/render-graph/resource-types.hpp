#pragma once
#include <cstdint>
#include <unordered_map>
#include <limits>
#include <assert.h>
#include <string>
#include <optional>
#include <vector>
#include <algorithm>
#include <variant>

enum class ResourceState
{
    ShaderWrite,
    ShaderRead,
    Indirect,
    Undefined
};

struct SubResourceRange
{
    uint32_t baseMip = 0, mipCount = UINT32_MAX,
             baseLayer = 0, layerCount = UINT32_MAX;
    ResourceState from = ResourceState::Undefined;

    bool isAll() const
    {
        return baseMip == 0 && mipCount == UINT32_MAX && baseLayer == 0 && layerCount == UINT32_MAX;
    }

    SubResourceRange resolve(uint32_t texMips, uint32_t texLayers) const
    {
        SubResourceRange r = *this;

        r.baseMip = std::min(r.baseMip, texMips);
        r.baseLayer = std::min(r.baseLayer, texLayers);
        if (r.mipCount == UINT32_MAX)
            r.mipCount = std::max(texMips - baseMip, 1u);
        if (r.layerCount == UINT32_MAX)
            r.layerCount = std::max(texLayers - baseLayer, 1u);
        return r;
    }
};
struct TextureState
{

    TextureState(uint32_t mipCount = 1, uint32_t layers = 1) : perSub(mipCount * layers, ResourceState::Undefined), m_mipCount(mipCount), m_layers(layers)
    {
    }

    uint32_t mipCount() const { return m_mipCount; }
    uint32_t layerCount() const { return m_layers; }
    ResourceState get(uint32_t sub) const { return uniform ? all : perSub[sub]; }
    uint32_t subIndex(uint32_t mip, uint32_t layer) const { return layer * m_mipCount + mip; }
    void updateSub(uint32_t sub, ResourceState state)
    {
        if (sub >= (uint32_t)perSub.size())
        {
            return;
        }

        if (get(sub) == state)
            return;
        perSub[sub] = state;
        checkUniform();

        if (uniform)
        {
            all = state;
        }
    }

    void updateRange(ResourceState state, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)
    {
        uint32_t maxMip = baseMip + mipCount;
        uint32_t maxLayer = baseLayer + layerCount;

        assert(maxMip <= m_mipCount);
        assert(maxLayer <= m_layers);

        for (uint32_t mip = baseMip; mip < maxMip; mip++)
        {
            for (uint32_t layer = baseLayer; layer < maxLayer; layer++)
            {
                uint32_t sub = subIndex(mip, layer);

                if (perSub[sub] == state)
                    continue;
                perSub[sub] = state;
            }
        }

        checkUniform();
    }
    void updateAll(ResourceState state)
    {

        all = state;
        uniform = true;
        std::fill(perSub.begin(), perSub.end(), state);
    }
    const std::vector<ResourceState> &subState() const { return perSub; }
    bool isUniform() const { return uniform; }

private:
    void checkUniform()
    {
        for (int i = 1; i < (int)perSub.size(); i++)
        {

            if (perSub[i - 1] != perSub[i])
            {
                uniform = false;
                return;
            }
        }
        uniform = true;
        all = perSub[0];
    }

private:
    uint32_t m_mipCount = 1, m_layers = 1;
    ResourceState all = ResourceState::Undefined;

    std::vector<ResourceState> perSub;
    bool uniform = true;
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
    TextureState state;
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
    SubResourceRange range{};
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
