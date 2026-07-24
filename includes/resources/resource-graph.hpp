#pragma once
#include "../graph.hpp"
#include <vector>
#include <unordered_map>
#include <functional>
#include <iomanip>
#include <any>
#include <variant>

using ResourceID = uint32_t;

enum class ResourceType
{
    Buffer,
    Texture,
    DepthBuffer
};

struct ResourceDesc
{
    ResourceID id;
    std::string name;
    size_t sizeBytes;
    ResourceType type;
    bool transient;
};

struct ResourceContext
{
    std::unordered_map<ResourceID, std::any> data;

    template <typename T>
    T &get(ResourceID id)
    {
        return std::any_cast<T &>(data[id]);
    }
    template <typename T>
    void set(ResourceID id, T value)
    {
        data[id] = std::move(value);
    }
};
struct GraphNode
{
    std::string name;
    std::vector<ResourceID> produces;
    std::vector<ResourceID> consumes;
    std::function<void(ResourceContext &)> execute;
};

struct Barrier
{
    ResourceID resourceId;
    std::string fromState;
    std::string toState;
};

struct CompiledPipeline
{
    using Step = std::variant<int, Barrier>;

    std::vector<Step> steps;
};

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
struct ResourceLifetime
{
    ResourceID id;
    int createPass;
    int lastUsePass;
    bool transient;
};

struct MemoryBlock
{
    int id;
    size_t sizeBytes;
    int lastUsedAtPass;
};

class ResourceGraph
{
    std::vector<GraphNode> m_nodes;
    std::unordered_map<ResourceID, ResourceDesc> m_resources;
    Graph m_depGraph;

public:
    void declareResource(ResourceDesc desc)
    {
        m_resources[desc.id] = desc;
    }

    void addNode(GraphNode node)
    {
        m_nodes.emplace_back(std::move(node));
    }

    CompiledPipeline compile()
    {
        m_depGraph = Graph();
        std::unordered_map<ResourceID, int> producerOf;

        for (int i = 0; i < m_nodes.size(); i++)
        {
            m_depGraph.addNode(i, m_nodes[i].name);

            for (auto &rid : m_nodes[i].produces)
            {
                if (producerOf.count(rid))
                {
                    throw std::runtime_error("Resource " + m_resources[rid].name + " has two producers");
                }

                producerOf[rid] = i;
            }
        }

        for (int i = 0; i < m_nodes.size(); i++)
        {
            for (auto &rid : m_nodes[i].consumes)
            {
                if (!producerOf.count(rid))
                {
                    throw std::runtime_error("Resource " + m_resources[rid].name + " has no producers");
                }

                int producer = producerOf[rid];
                if (producer != i && !m_depGraph.hasEdge(producer, i))
                {
                    m_depGraph.addEdge(producer, i);
                }
            }
        }

        auto cycles = m_depGraph.findCycles();
        if (!cycles.empty())
            throw std::runtime_error("Cycle in resource graph");

        auto order = m_depGraph.topoSort();
        CompiledPipeline pipeline;

        std::unordered_map<ResourceID, std::string> resourceState;
        for (auto &[rid, _] : m_resources)
        {
            resourceState[rid] = "Uninitialized";
        };
        for (auto &nodeIdx : order)
        {
            auto &node = m_nodes[nodeIdx];

            for (auto &rid : node.consumes)
            {
                if (resourceState[rid] == "Produced")
                {
                    resourceState[rid] = "Consumed";
                    pipeline.steps.push_back(Barrier{rid, "Produced", "Consumed"});
                };
            }

            pipeline.steps.push_back((int)nodeIdx);

            for (auto &rid : node.produces)
            {
                resourceState[rid] = "Produced";
            }
        }

        return pipeline;
    }
    void execute(const CompiledPipeline &pipeline)
    {
        ResourceContext ctx;
        for (auto &step : pipeline.steps)
        {
            std::visit(
                overloaded{
                    [&](int nodeIdx)
                    {
                        auto &node = m_nodes[nodeIdx];
                        std::cout << "[Execute] " << node.name << "\n";
                        node.execute(ctx);
                    },
                    [&](const Barrier &b)
                    {
                        std::cout << "[Barrier] " << m_resources[b.resourceId].name
                                  << ": " << b.fromState
                                  << " → " << b.toState << "\n";
                    }},
                step);
        }
    }

    std::vector<uint32_t> executionOrder()
    {
        return m_depGraph.topoSort();
    }
    GraphNode *getNode(int i)
    {
        if (i >= m_nodes.size())
            return nullptr;

        return &m_nodes[i];
    }

    GraphNode *getProducer(ResourceID rid)
    {
        for (auto &node : m_nodes)
        {
            for (auto produceId : node.produces)
            {
                if (produceId == rid)
                {
                    return &node;
                }
            }
        }

        return nullptr;
    };

    const std::vector<GraphNode *> getConsumers(ResourceID rid)
    {
        std::unordered_set<int> consumerNodeIdx;
        std::vector<GraphNode *> consumers;
        for (int i = 0; i < m_nodes.size(); i++)
        {
            for (auto cid : m_nodes[i].consumes)
            {
                if (cid == rid && !consumerNodeIdx.count(i))
                {
                    consumerNodeIdx.insert(i);
                    consumers.push_back(&m_nodes[i]);
                }
            }
        }

        return consumers;
    }

    int getProducerIndex(ResourceID rid)
    {

        for (int i = 0; i < m_nodes.size(); i++)
        {
            for (auto &pid : m_nodes[i].produces)
            {
                if (pid == rid)
                {
                    return i;
                }
            }
        }

        return -1;
    }

    const std::vector<int> getConsumerIndices(ResourceID rid)
    {
        std::unordered_set<int> consumerNodeIdx;
        std::vector<int> consumers;
        for (int i = 0; i < m_nodes.size(); i++)
        {
            for (auto cid : m_nodes[i].consumes)
            {
                if (cid == rid && !consumerNodeIdx.count(i))
                {
                    consumerNodeIdx.insert(i);
                    consumers.push_back(i);
                }
            }
        }

        return consumers;
    }

    void printResourceFlow()
    {
        for (auto &[rid, desc] : m_resources)
        {
            auto *producer = getProducer(rid);
            auto consumers = getConsumers(rid);

            std::cout << desc.name << ": ";
            std::cout << (producer ? producer->name : "[none]") << " → ";
            if (consumers.empty())
                std::cout << "[output]";
            for (auto *c : consumers)
                std::cout << c->name << " ";
            std::cout << "\n";
        };
    };

    const std::vector<ResourceLifetime> computeLifetimes()
    {
        auto order = m_depGraph.topoSort();
        std::unordered_map<int, int> nodePos;

        for (int i = 0; i < order.size(); i++)
        {
            nodePos[order[i]] = i;
        }

        std::vector<ResourceLifetime> lifetimes;

        for (auto &[rid, desc] : m_resources)
        {

            int produceIdx = getProducerIndex(rid);
            if (produceIdx < 0)
                continue;

            auto consumers = getConsumerIndices(rid);

            int createPos = nodePos[produceIdx];
            int lastUsePos = createPos;

            for (int c : consumers)
            {
                lastUsePos = std::max(lastUsePos, nodePos[c]);
            }

            lifetimes.push_back({rid, createPos, lastUsePos, desc.transient});
        };

        return lifetimes;
    };

    void printLifetimes()
    {

        auto order = m_depGraph.topoSort();
        auto lifetimes = computeLifetimes();
        int numPasses = order.size();

        std::cout << std::setw(16) << " ";
        for (int i = 0; i < numPasses; i++)
            std::cout << std::setw(12) << m_nodes[order[i]].name << " ";
        std::cout << "\n";

        for (auto &lt : lifetimes)
        {
            std::cout << std::setw(16) << m_resources[lt.id].name;
            for (int i = 0; i < numPasses; i++)
            {
                if (i == lt.createPass)
                    std::cout << std::setw(12) << "C---";
                else if (i == lt.lastUsePass)
                    std::cout << std::setw(12) << "---D";
                else if (i > lt.createPass && i < lt.lastUsePass)
                    std::cout << std::setw(12) << "----";
                else
                    std::cout << std::setw(12) << "    ";
            };
            std::cout << "\n";
        };
    };

    size_t peakMemoryBytes()
    {
        auto order = m_depGraph.topoSort();
        auto lifetimes = computeLifetimes();
        size_t peek = 0;
        for (int i = 0; i < order.size(); i++)
        {
            size_t live = 0;
            for (auto &lt : lifetimes)
            {
                if (lt.createPass <= i && lt.lastUsePass >= i)
                {
                    live += m_resources[lt.id].sizeBytes;
                }
            }
            peek = std::max(peek, live);
        }

        return peek;
    }
    const std::vector<ResourceDesc *> getDeadZone(int afterPass)
    {
        auto lifetimes = computeLifetimes();
        std::vector<ResourceDesc *> deadZones;
        for (auto &lt : lifetimes)
        {
            if (lt.createPass <= afterPass && lt.lastUsePass >= afterPass)
                continue;

            deadZones.push_back(&m_resources[lt.id]);
        }

        return deadZones;
    };

    std::unordered_map<ResourceID, int> assignAliases()
    {
        auto lifetimes = computeLifetimes();
        // Sort in ascending order unless you stand a chance of getting no alias
        std::sort(lifetimes.begin(), lifetimes.end(),
                  [](const ResourceLifetime &a, const ResourceLifetime &b)
                  {
                      return a.createPass < b.createPass;
                  });
        std::unordered_map<ResourceID, int> assignments;

        std::vector<MemoryBlock> blocks;

        for (auto &lt : lifetimes)
        {
            auto needed = m_resources.at(lt.id).sizeBytes;

            if (!lt.transient)
            {
                int blockId = blocks.size();
                blocks.push_back({blockId, needed, lt.lastUsePass});
                assignments[lt.id] = blockId;
                continue;
            }

            int reusedBlock = -1;

            for (auto &block : blocks)
            {

                if (block.lastUsedAtPass < lt.createPass && block.sizeBytes >= needed)
                {
                    if (reusedBlock < 0 || block.sizeBytes < blocks[reusedBlock].sizeBytes)
                    {
                        reusedBlock = block.id;
                    }
                }
            }

            if (reusedBlock >= 0)
            {
                assignments[lt.id] = reusedBlock;
                blocks[reusedBlock].lastUsedAtPass = lt.lastUsePass;
            }
            else
            {
                int blockId = blocks.size();
                blocks.push_back({blockId, needed, lt.lastUsePass});
                assignments[lt.id] = blockId;
            }
        }

        return assignments;
    };

    void printAliasingReport()
    {
        auto aliases = assignAliases();

        std::unordered_map<int, std::vector<ResourceID>> blockContents;

        for (auto &[rid, blockId] : aliases)
        {
            blockContents[blockId].push_back(rid);
        };

        size_t totalSize = 0, aliasedSize = 0;

        for (auto &[rid, desc] : m_resources)
            totalSize += desc.sizeBytes;

        for (auto &[blockId, rids] : blockContents)
        {
            if (rids.size() > 1)
            {
                std::cout << "Block " << blockId << " (aliased): ";
                for (ResourceID rid : rids)
                    std::cout << m_resources[rid].name << " ";
                aliasedSize += m_resources[rids[0]].sizeBytes * (rids.size() - 1);
                std::cout << "\n";
            }
        }

        std::cout << "Memory savings: " << aliasedSize / (1024 * 1024)
                  << " MB (" << (100 * aliasedSize / totalSize) << "%)\n";
    };
};