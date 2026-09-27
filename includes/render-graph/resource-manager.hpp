#pragma once
#include <cstdint>
#include <unordered_map>
#include <limits>
#include <assert.h>
#include <string>
#include <optional>
#include <vector>
#include "ring-buffer.hpp"

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
constexpr uint32_t g_MAX_FRAME_COUNT = 2;

class ResourceManager
{
    template <typename T, typename Desc>
    struct PhysicalResource
    {
        ResourceInstanceType instanceType;
        ResourceLifetimeType lifetime;
        Desc descriptor;
        RingBuffer<T> resources;
        uint64_t instanceCount;
    };
    using PhysicalTexture = PhysicalResource<GPUTexture, GPUTextureDesc>;
    using PhysicalBuffer = PhysicalResource<GPUBuffer, GPUBufferDesc>;

    std::vector<PhysicalTexture> m_textures;
    std::vector<PhysicalBuffer> m_buffers;

    // Pending for build
    std::vector<TextureHandle> m_pendingTextures;
    std::vector<BufferHandle> m_pendingBuffers;
    std::unordered_map<std::string, TextureHandle> m_nameToTexture;
    std::unordered_map<std::string, BufferHandle> m_nameToBuffer;
    uint32_t m_currentFrameIdx = 0;

public:
    TextureHandle createTexture(std::string name, GPUTextureDesc desc, ResourceLifetimeType lifetime)
    {
        return createTexture(name, desc, lifetime, ResourceInstanceType::Single, 1);
    };
    TextureHandle createPerFrameTexture(std::string name, GPUTextureDesc desc, ResourceLifetimeType lifetime)
    {
        return createTexture(name, desc, lifetime, ResourceInstanceType::PerFrame, g_MAX_FRAME_COUNT);
    };
    TextureHandle createTemporalTexture(std::string name, GPUTextureDesc desc, ResourceLifetimeType lifetime, uint32_t historyCount)
    {

        assert(historyCount > 1);
        return createTexture(name, desc, lifetime, ResourceInstanceType::Temporal, historyCount);
    };

    BufferHandle createBuffer(std::string name, GPUBufferDesc desc)
    {
        return createBuffer(name, desc, ResourceLifetimeType::Persistent, ResourceInstanceType::Single, 1);
    };
    BufferHandle createPerFrameBuffer(std::string name, GPUBufferDesc desc)
    {
        return createBuffer(name, desc, ResourceLifetimeType::Persistent, ResourceInstanceType::PerFrame, g_MAX_FRAME_COUNT);
    };
    BufferHandle createTemporalBuffer(std::string name, GPUBufferDesc desc, uint32_t historyCount)
    {

        assert(historyCount > 1);

        return createBuffer(name, desc, ResourceLifetimeType::Persistent, ResourceInstanceType::Temporal, historyCount);
    };

    bool isValid(TextureHandle handle)
    {
        return handle.isValid() && handle.id < static_cast<HandleId>(m_textures.size());
    };
    bool isValid(BufferHandle handle)
    {
        return handle.isValid() && handle.id < static_cast<HandleId>(m_buffers.size());
    };
    std::optional<TextureHandle> getTextureHandle(std::string name)
    {
        auto it = m_nameToTexture.find(name);

        if (it == m_nameToTexture.end())
        {
            return std::nullopt;
        }

        return it->second;
    }
    std::optional<BufferHandle> getBufferHandle(std::string name)
    {
        auto it = m_nameToBuffer.find(name);

        if (it == m_nameToBuffer.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    GPUTexture *get(TextureHandle handle)
    {
        if (!isValid(handle))
        {
            return nullptr;
        }

        return m_textures[handle.id].resources.get(m_currentFrameIdx);
    };
    GPUBuffer *get(BufferHandle handle)
    {
        if (!isValid(handle))
        {
            return nullptr;
        }

        return m_buffers[handle.id].resources.get(m_currentFrameIdx);
    };

    GPUBuffer *current(BufferHandle handle)
    {
        return get(handle);
    };
    GPUTexture *current(TextureHandle handle)
    {
        return get(handle);
    };

    GPUTexture *previous(TextureHandle handle)
    {
        if (!isValid(handle))
            return nullptr;

        auto &resource = m_textures[handle.id];

        assert(resource.instanceType == ResourceInstanceType::Temporal);

        return resource.resources.get(m_currentFrameIdx > 0 ? m_currentFrameIdx - 1 : 0);
    };
    GPUBuffer *previous(BufferHandle handle)
    {
        if (!isValid(handle))
            return nullptr;

        auto &resource = m_buffers[handle.id];

        assert(resource.instanceType == ResourceInstanceType::Temporal);

        return resource.resources.get(m_currentFrameIdx > 0 ? m_currentFrameIdx - 1 : 0);
    };

    void buildTextures(uint32_t frameWidth, uint32_t frameHeight)
    {
        for (auto &handle : m_pendingTextures)
        {
            if (!isValid(handle))
                continue;

            auto &resource = m_textures[handle.id];

            if (resource.lifetime == ResourceLifetimeType::Transient)
            {
                continue;
                // Transient will come later
            }

            resource.resources.create(resource.instanceCount,
                                      [](uint32_t frameIdx)
                                      {
                                          return GPUTexture{};
                                      });
        }

        m_pendingTextures.clear();
    }
    void buildBuffers(uint32_t frameWidth, uint32_t frameHeight)
    {
        for (auto &handle : m_pendingBuffers)
        {
            if (!isValid(handle))
                continue;

            auto &resource = m_buffers[handle.id];

            if (resource.lifetime == ResourceLifetimeType::Transient)
            {
                continue;
                // Transient will come later
            }

            resource.resources.create(resource.instanceCount,
                                      [](uint32_t frameIdx)
                                      {
                                          return GPUBuffer{};
                                      });
        }

        m_pendingTextures.clear();
    }

private:
    TextureHandle createTexture(std::string name, GPUTextureDesc desc, ResourceLifetimeType lifetime, ResourceInstanceType instanceType, uint32_t instanceCount)
    {
        if (auto it = m_nameToTexture.find(name); it != m_nameToTexture.end())
        {
            return it->second;
        }

        auto id = static_cast<HandleId>(m_textures.size());

        PhysicalTexture texture;
        texture.descriptor = desc;
        texture.lifetime = lifetime;
        texture.instanceType = instanceType;
        texture.instanceCount = instanceCount;

        m_textures.push_back(std::move(texture));
        TextureHandle handle{id};

        m_nameToTexture[name] = handle;

        m_pendingTextures.push_back(handle);
        return handle;
    };
    BufferHandle createBuffer(std::string name, GPUBufferDesc desc, ResourceLifetimeType lifetime, ResourceInstanceType instanceType, uint32_t instanceCount)
    {
        if (auto it = m_nameToBuffer.find(name); it != m_nameToBuffer.end())
        {
            return it->second;
        }

        auto id = static_cast<HandleId>(m_buffers.size());

        PhysicalBuffer buffer;
        buffer.descriptor = desc;
        buffer.lifetime = lifetime;
        buffer.instanceType = instanceType;
        buffer.instanceCount = instanceCount;

        m_buffers.push_back(std::move(buffer));
        BufferHandle handle{id};

        m_nameToBuffer[name] = handle;

        m_pendingBuffers.push_back(handle);
        return handle;
    };
};
