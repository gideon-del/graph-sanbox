#pragma once
#include "../graph.hpp"
#include "resource-manager.hpp"
#include <optional>
#include <vector>
#include <functional>

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

enum class ResourceState
{
    ShaderWrite,
    ShaderRead,
    Indirect
};

template <typename T>
struct ResourceAccess
{
    Handle<T> logicalResource;
    ResourceState state;
};

using TextureAccess = ResourceAccess<LogicalTexture>;
using BufferAccess = ResourceAccess<LogicalBuffer>;

class RenderGraphBuilder;

using RenderPassExecute = std::function<void(const ResourceManager &)>;
class RenderPass
{

public:
    RenderPass(uint32_t index, std::string name, RenderGraphBuilder &renderGraph);

    RenderPass &read(const LogicalTextureHandle &handle, const ResourceState &state);
    RenderPass &read(const TemporalLogicalTexture &temporal, const ResourceState &state);

    RenderPass &read(const TemporalLogicalBuffer &temporal, const ResourceState &state);
    RenderPass &read(const LogicalBufferHandle &handle, const ResourceState &state);

    RenderPass &write(const LogicalTextureHandle &handle, const ResourceState &state);
    RenderPass &write(const TemporalLogicalTexture &temporal, const ResourceState &state);

    RenderPass &write(const TemporalLogicalBuffer &temporal, const ResourceState &state);
    RenderPass &write(const LogicalBufferHandle &handle, const ResourceState &state);

    RenderPass &readHistory(const TemporalLogicalBuffer &temporal, const ResourceState &state);
    RenderPass &readHistory(const TemporalLogicalTexture &temporal, const ResourceState &state);

    RenderPass &setExecuteFn(RenderPassExecute execute);

    const std::vector<TextureAccess> &getTextureAccess() const { return m_textures; }
    const std::vector<BufferAccess> &getBufferAccess() const { return m_buffers; }
    const RenderPassExecute &getExecute() { return m_execute; }
    const std::string &name() const { return m_name; }

private:
    uint32_t m_index;
    std::string m_name;
    std::vector<TextureAccess> m_textures;
    std::vector<BufferAccess> m_buffers;
    RenderPassExecute m_execute;

    RenderGraphBuilder &m_renderGraph;
};

class RenderGraphBuilder
{

public:
    RenderGraphBuilder(ResourceManager &resourceManager);
    LogicalTextureHandle createTexture(const std::string &name);
    LogicalBufferHandle createBuffer(const std::string &name);

    TemporalLogicalTexture createTemporalTexture(const std::string &name);
    TemporalLogicalBuffer createTemporalBuffer(const std::string &name);

    LogicalTexture *getTexture(const LogicalTextureHandle &handle);
    LogicalBuffer *getBuffer(const LogicalBufferHandle &handle);

    RenderPass &addRenderPass(std::string name);

    void printResourceFlow();
    void compile();

private:
    LogicalTextureHandle createTexture(const std::string &name, bool history);
    LogicalBufferHandle createBuffer(const std::string &name, bool history);

private:
    Graph m_graph;
    ResourceManager &m_resourceManager;
    std::vector<LogicalTexture> m_logicalTextures;
    std::vector<LogicalBuffer> m_logicalBuffers;

    std::vector<RenderPass> m_renderPasses;
    std::unordered_map<std::string, uint32_t> m_nameToPassIndex;

    std::vector<uint32_t> m_executionOrder;
};
