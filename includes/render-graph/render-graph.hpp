#pragma once
#include "../graph.hpp"
#include "resource-manager.hpp"
#include <optional>
#include <vector>
#include <functional>
#include <variant>
#include "resource-types.hpp"

class RenderGraphBuilder;

using RenderPassExecute = std::function<void(const ResourceManager &)>;
class RenderPass
{

public:
    RenderPass(uint32_t index, std::string name, RenderGraphBuilder &renderGraph);

    RenderPass &read(const LogicalTextureHandle &handle, const ResourceState &state, SubResourceRange range = SubResourceRange{})
    {
        return read(handle, state, false, range);
    };
    RenderPass &read(const TemporalLogicalTexture &temporal, const ResourceState &state, SubResourceRange range = SubResourceRange{});

    RenderPass &read(const TemporalLogicalBuffer &temporal, const ResourceState &state);
    RenderPass &read(const LogicalBufferHandle &handle, const ResourceState &state)
    {
        return read(handle, state, false);
    };

    RenderPass &write(const LogicalTextureHandle &handle, const ResourceState &state, SubResourceRange range = SubResourceRange{});
    RenderPass &write(const TemporalLogicalTexture &temporal, const ResourceState &state, SubResourceRange range = SubResourceRange{});

    RenderPass &write(const TemporalLogicalBuffer &temporal, const ResourceState &state);
    RenderPass &write(const LogicalBufferHandle &handle, const ResourceState &state);

    RenderPass &readHistory(const TemporalLogicalBuffer &temporal, const ResourceState &state);
    RenderPass &readHistory(const TemporalLogicalTexture &temporal, const ResourceState &state, SubResourceRange range = SubResourceRange{});

    RenderPass &setExecuteFn(RenderPassExecute execute);

    const std::vector<TextureAccess> &getTextureAccess() const { return m_textures; }
    const std::vector<BufferAccess> &getBufferAccess() const { return m_buffers; }
    const RenderPassExecute &getExecute() { return m_execute; }
    const std::string &name() const { return m_name; }
    const uint32_t index() const { return m_index; }

private:
    RenderPass &read(const LogicalTextureHandle &handle, const ResourceState &state, bool history, SubResourceRange range);
    RenderPass &read(const LogicalBufferHandle &handle, const ResourceState &state, bool history);

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

    const CompiledRenderGraph compile();
    void printResourceFlow();
    void printPassOrder();
    void printResourceLifetime();

    const PhysicalTextureLifetime &lifetimes() const
    {
        return m_lifetime;
    }
    const std::vector<uint32_t> &executionOrder() const { return m_executionOrder; }
    uint32_t orderOf(const std::string &passName) const;

private:
    LogicalTextureHandle createTexture(const std::string &name, bool history);
    LogicalBufferHandle createBuffer(const std::string &name, bool history);
    const PhysicalTextureLifetime calculateLifetimes() const;
    const CompiledRenderGraph::Step buildRenderPassStep(int passIdx);

private:
    Graph m_graph;
    ResourceManager &m_resourceManager;
    std::vector<LogicalTexture> m_logicalTextures;
    std::vector<LogicalBuffer> m_logicalBuffers;

    std::vector<RenderPass> m_renderPasses;
    std::unordered_map<std::string, uint32_t> m_nameToPassIndex;

    std::vector<uint32_t> m_executionOrder;
    PhysicalTextureLifetime m_lifetime;
};
