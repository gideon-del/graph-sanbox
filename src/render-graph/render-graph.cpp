#include "render-graph/render-graph.hpp"
#include <fstream>
#include <sstream>
#include <string>

RenderGraphBuilder::RenderGraphBuilder(ResourceManager &resourceManager) : m_resourceManager(resourceManager)
{
}

LogicalTextureHandle RenderGraphBuilder::createTexture(const std::string &name, bool history)
{
    auto physicalTexture = m_resourceManager.getTextureHandle(name);

    if (!physicalTexture)
    {
        throw std::runtime_error(std::format("Failed to find texture with name '{}'", name));
    }

    auto id = static_cast<HandleId>(m_logicalTextures.size());

    LogicalTexture logicalTexture{
        .name = name,
        .physicalHandle = *physicalTexture,
        .versions = {ResourceVersion{}},
        .history = history,
    };

    m_logicalTextures.push_back(logicalTexture);

    auto handle = LogicalTextureHandle{id};
    return handle;
}

LogicalTextureHandle RenderGraphBuilder::createTexture(const std::string &name)
{
    return createTexture(name, false);
}

LogicalBufferHandle RenderGraphBuilder::createBuffer(const std::string &name, bool history)
{

    auto physicalHandle = m_resourceManager.getBufferHandle(name);

    if (!physicalHandle)
    {
        throw std::runtime_error(std::format("Failed to find buffer with name '{}'", name));
    }
    auto id = static_cast<HandleId>(m_logicalBuffers.size());

    LogicalBuffer logicalBuffer{
        .name = name,
        .physicalHandle = *physicalHandle,
        .versions = {ResourceVersion{}},
        .history = history,
    };

    m_logicalBuffers.push_back(logicalBuffer);

    auto handle = LogicalBufferHandle{id};
    return handle;
}

LogicalBufferHandle RenderGraphBuilder::createBuffer(const std::string &name)
{

    return createBuffer(name, false);
}

TemporalLogicalTexture RenderGraphBuilder::createTemporalTexture(const std::string &name)
{

    auto current = createTexture(name, false);
    auto previous = createTexture(name, true);

    return {
        .current = current,
        .previous = previous};
}
TemporalLogicalBuffer RenderGraphBuilder::createTemporalBuffer(const std::string &name)
{

    auto current = createBuffer(name, false);
    auto previous = createBuffer(name, true);

    return {
        .current = current,
        .previous = previous};
}
LogicalTexture *RenderGraphBuilder::getTexture(const LogicalTextureHandle &handle)
{
    if (!handle.isValid() || handle.id >= m_logicalTextures.size())
    {
        return nullptr;
    }

    return &m_logicalTextures[handle.id];
}

LogicalBuffer *RenderGraphBuilder::getBuffer(const LogicalBufferHandle &handle)
{
    if (!handle.isValid() || handle.id >= m_logicalBuffers.size())
    {
        return nullptr;
    }

    return &m_logicalBuffers[handle.id];
}
RenderPass &RenderGraphBuilder::addRenderPass(std::string name)
{
    if (auto it = m_nameToPassIndex.find(name); it != m_nameToPassIndex.end())
    {
        return m_renderPasses[it->second];
    }

    uint32_t index = m_renderPasses.size();

    m_renderPasses.push_back(RenderPass{index, name, *this});

    m_nameToPassIndex[name] = index;
    return m_renderPasses.back();
}

RenderPass::RenderPass(uint32_t index, std::string name, RenderGraphBuilder &renderGraph)
    : m_renderGraph(renderGraph),
      m_index(index),
      m_name(name)
{
}

RenderPass &RenderPass::read(const LogicalTextureHandle &handle, const ResourceState &state)
{
    auto logicalTexture = m_renderGraph.getTexture(handle);

    if (!logicalTexture)
    {
        return *this;
    }

    if (logicalTexture->versions.empty())
    {
        logicalTexture->versions.push_back({});
    }

    auto &currentVersion = logicalTexture->versions.back();

    currentVersion.consumers.push_back(m_index);

    m_textures.push_back({.logicalResource = handle,
                          .state = state});
    return *this;
}

RenderPass &RenderPass::read(const LogicalBufferHandle &handle, const ResourceState &state)
{
    auto logicalBuffer = m_renderGraph.getBuffer(handle);

    if (!logicalBuffer)
    {
        return *this;
    }

    if (logicalBuffer->versions.empty())
    {
        logicalBuffer->versions.push_back({});
    }

    auto &currentVersion = logicalBuffer->versions.back();

    currentVersion.consumers.push_back(m_index);

    m_buffers.push_back({.logicalResource = handle,
                         .state = state});
    return *this;
}

RenderPass &RenderPass::read(const TemporalLogicalTexture &temporal, const ResourceState &state)
{
    return read(temporal.current, state);
}
RenderPass &RenderPass::readHistory(const TemporalLogicalTexture &temporal, const ResourceState &state)
{
    return read(temporal.previous, state);
}
RenderPass &RenderPass::read(const TemporalLogicalBuffer &temporal, const ResourceState &state)
{
    return read(temporal.current, state);
}
RenderPass &RenderPass::readHistory(const TemporalLogicalBuffer &temporal, const ResourceState &state)
{
    return read(temporal.previous, state);
}
RenderPass &RenderPass::setExecuteFn(RenderPassExecute execute)
{
    m_execute = execute;

    return *this;
}

RenderPass &RenderPass::write(const LogicalTextureHandle &handle, const ResourceState &state)
{
    auto logicalTexture = m_renderGraph.getTexture(handle);

    if (!logicalTexture)
    {
        return *this;
    }

    if (logicalTexture->versions.empty())
    {
        logicalTexture->versions.push_back({});
    }

    if (logicalTexture->versions.back().producer)
    {
        logicalTexture->versions.push_back({});
    }

    auto &current = logicalTexture->versions.back();

    current.producer = m_index;

    m_textures.push_back({.logicalResource = handle,
                          .state = state});

    return *this;
}

RenderPass &RenderPass::write(const LogicalBufferHandle &handle, const ResourceState &state)
{
    auto logicalBuffer = m_renderGraph.getBuffer(handle);

    if (!logicalBuffer)
    {
        return *this;
    }

    if (logicalBuffer->versions.empty())
    {
        logicalBuffer->versions.push_back({});
    }

    if (logicalBuffer->versions.back().producer)
    {
        logicalBuffer->versions.push_back({});
    }
    auto &current = logicalBuffer->versions.back();

    current.producer = m_index;

    m_buffers.push_back({.logicalResource = handle,
                         .state = state});

    return *this;
}

RenderPass &RenderPass::write(const TemporalLogicalBuffer &temporal, const ResourceState &state)
{
    return write(temporal.current, state);
}
RenderPass &RenderPass::write(const TemporalLogicalTexture &temporal, const ResourceState &state)
{
    return write(temporal.current, state);
}

void RenderGraphBuilder::printResourceFlow()
{

    std::cout << " ========Texture Resource Flow====== " << std::endl;

    std::stringstream ss;

    for (auto &texture : m_logicalTextures)
    {
        ss << texture.name << "\n";

        for (int i = 0; i < texture.versions.size(); i++)
        {

            ss << "|\n";
            ss << "|\n";
            ss << "|\n";
            auto &version = texture.versions[i];

            ss << " --- Verion " << i << "\n";
            ss << std::setw(20) << "Producer:  ";
            if (version.producer)
            {
                ss << m_renderPasses[*version.producer].name();
            }
            else
            {
                ss << "None";
            }

            ss << "\n";
            ss << std::setw(20) << "Consumers: ";

            if (version.consumers.empty())
            {
                ss << "None";
            }
            else
            {
                for (auto &passIdx : version.consumers)
                {
                    ss << m_renderPasses[passIdx].name() << " ";
                }
            }

            ss << "\n";
        }

        ss << "\n";
    }

    std::cout << ss.str() << std::endl;

    ss.flush();

    std::cout << " ========Buffer Resource Flow====== " << std::endl;
};

void RenderGraphBuilder::compile()
{
    m_graph = Graph();

    for (auto &pass : m_renderPasses)
    {
        m_graph.addNode(pass.index());
    }

    for (auto &logicalTexture : m_logicalTextures)
    {
        std::optional<int> previousVersionIdx = std::nullopt;

        if (logicalTexture.history)
        {
            continue;
        }
        for (int i = 0; i < logicalTexture.versions.size(); i++)
        {

            auto &currentVersion = logicalTexture.versions[i];

            bool hasProducer = currentVersion.producer.has_value();

            if (!hasProducer)
            {
                throw std::runtime_error(std::format("{} does not have a producer", logicalTexture.name));
            }

            auto producerIdx = *currentVersion.producer;

            for (auto &consumer : currentVersion.consumers)
            {
                if (consumer != producerIdx)
                {
                    m_graph.addEdge(producerIdx, consumer);
                }
            }

            if (previousVersionIdx)
            {
                auto &previousVersion = logicalTexture.versions[*previousVersionIdx];
                m_graph.addEdge(*previousVersion.producer, producerIdx);

                for (auto &consumerIdx : previousVersion.consumers)
                {
                    if (consumerIdx != producerIdx)
                    {
                        m_graph.addEdge(consumerIdx, producerIdx);
                    }
                }
            }

            previousVersionIdx = i;
        }
    }

    for (auto &logicalBuffer : m_logicalBuffers)
    {
        std::optional<int> previousVersionIdx = std::nullopt;

        if (logicalBuffer.history)
        {
            continue;
        }
        for (int i = 0; i < logicalBuffer.versions.size(); i++)
        {

            auto &currentVersion = logicalBuffer.versions[i];

            bool hasProducer = currentVersion.producer.has_value();

            if (!hasProducer)
            {
                throw std::runtime_error(std::format("{} does not have a producer", logicalBuffer.name));
            }

            auto producerIdx = *currentVersion.producer;

            for (auto &consumer : currentVersion.consumers)
            {
                if (consumer != producerIdx)
                {
                    m_graph.addEdge(producerIdx, consumer);
                }
            }
            if (previousVersionIdx)
            {
                auto &previousVersion = logicalBuffer.versions[*previousVersionIdx];
                m_graph.addEdge(*previousVersion.producer, producerIdx);

                for (auto &consumerIdx : previousVersion.consumers)
                {
                    if (consumerIdx != producerIdx)
                    {
                        m_graph.addEdge(consumerIdx, producerIdx);
                    }
                }
            }

            previousVersionIdx = i;
        }
    }

    m_executionOrder = m_graph.topoSort();
}

void RenderGraphBuilder::printPassOrder()
{
    std::cout << "Render Pass Execution Order" << std::endl;
    std::stringstream ss;
    for (auto &passIdx : m_executionOrder)
    {
        ss << " --> " << m_renderPasses[passIdx].name();
    }

    std::cout << ss.str() << std::endl;
}
