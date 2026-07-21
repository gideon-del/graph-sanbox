#pragma once
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <assert.h>
#include <functional>
#include <unordered_set>
#include <queue>
#include <stack>
#include <iostream>

using NodeID = uint32_t;

class Graph
{
    std::unordered_map<NodeID, std::vector<NodeID>> m_adjacentList;

    std::unordered_map<NodeID, std::string> m_nodeLabels;
    std::string m_lastError;

public:
    void addNode(NodeID id, std::string label = "")
    {
        m_adjacentList.emplace(id, std::vector<NodeID>{});

        m_nodeLabels[id] = label.empty() ? std::to_string(id) : label;
    }
    std::string getLabel(NodeID id)
    {
        assert(hasNode(id));
        return m_nodeLabels[id];
    }
    bool hasNode(NodeID id)
    {
        return m_adjacentList.count(id) > 0;
    }

    bool hasEdge(NodeID from, NodeID to)
    {
        assert(hasNode(from));
        const auto &neighbors = m_adjacentList.at(from);
        return std::find(neighbors.begin(), neighbors.end(), to) != neighbors.end();
    }

    void addEdge(NodeID from, NodeID to)
    {
        if (hasEdge(from, to))
            return;

        m_adjacentList.at(from).push_back(to);
    }
    void removeNode(NodeID id)
    {
        m_adjacentList.erase(id);

        for (auto &[nodeId, neighbors] : m_adjacentList)
        {
            neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), id), neighbors.end());
        }
    }

    size_t nodeCount()
    {

        return m_adjacentList.size();
    }

    size_t edgeCount()
    {
        size_t count = 0;
        for (auto &[id, nbrs] : m_adjacentList)
            count += nbrs.size();
        return count;
    }

    const std::vector<NodeID> &neighbors(NodeID id)
    {
        assert(hasNode(id));

        return m_adjacentList.at(id);
    }

    void forEachNode(std::function<void(NodeID id)> fn) const
    {
        for (auto &[id, _] : m_adjacentList)
            fn(id);
    }

    std::vector<NodeID> bfs(NodeID id)
    {
        assert(hasNode(id));

        std::queue<NodeID> nodeQueue;
        std::vector<NodeID> sortedNodes;
        std::unordered_set<NodeID> visited;

        nodeQueue.push(id);
        visited.insert(id);
        while (nodeQueue.size() > 0)
        {
            NodeID currentNode = nodeQueue.front();

            for (auto &neighborNode : m_adjacentList.at(currentNode))
            {
                if (visited.find(neighborNode) == visited.end())
                {
                    nodeQueue.push(neighborNode);
                    visited.insert(neighborNode);
                }
            }

            sortedNodes.push_back(currentNode);
            nodeQueue.pop();
        }

        return sortedNodes;
    }

    std::vector<NodeID> dfs(NodeID id)
    {
        std::stack<NodeID> stack;
        std::vector<NodeID> order;
        std::unordered_set<NodeID> visited;

        stack.push(id);

        while (!stack.empty())
        {
            NodeID currentNode = stack.top();
            stack.pop();
            if (visited.find(currentNode) != visited.end())
                continue;
            order.push_back(currentNode);
            visited.insert(currentNode);

            auto &neighborNodes = m_adjacentList.at(currentNode);

            for (auto it = neighborNodes.rbegin(); it != neighborNodes.rend(); ++it)
            {
                if (!visited.count(*it))
                {
                    stack.push(*it);
                }
            }
        }

        return order;
    };

    std::vector<NodeID> bfsAll()
    {
        std::vector<NodeID> order;
        std::unordered_set<NodeID> globalVisited;

        forEachNode([&](NodeID id)
                    {
        if (globalVisited.count(id)) return;  
        auto component = bfs(id);          
            for (auto n : component)
                        {
                            if (!globalVisited.count(n))
                            {
                                globalVisited.insert(n);
                                order.push_back(n);
                            }
                        } });

        return order;
    }
    std::vector<NodeID> dfsAll()
    {
        std::vector<NodeID> order;
        std::unordered_set<NodeID> globalVisited;

        forEachNode([&](NodeID id)
                    {
                        if (globalVisited.count(id))
                            return;
                        auto component = dfs(id);
                        for (auto n : component)
                        {
                            if (!globalVisited.count(n))
                            {
                                globalVisited.insert(n);
                                order.push_back(n);
                            }
                        } });

        return order;
    }

    std::vector<NodeID> topoSort()
    {
        std::unordered_map<NodeID, int> inDegree;
        std::queue<NodeID> processedNode;
        std::vector<NodeID> order;
        for (auto &[id, _] : m_adjacentList)
        {
            inDegree[id] = 0;
        }

        for (auto &[_, neighborNodes] : m_adjacentList)
        {
            for (auto &node : neighborNodes)
            {
                inDegree[node]++;
            }
        }

        for (auto &[node, degree] : inDegree)
        {
            if (degree == 0)
            {
                processedNode.push(node);
            }
        }

        while (!processedNode.empty())
        {
            NodeID currentNode = processedNode.front();

            processedNode.pop();
            order.push_back(currentNode);

            for (auto neighbourNode : m_adjacentList.at(currentNode))
            {
                inDegree[neighbourNode]--;

                if (inDegree[neighbourNode] == 0)
                {
                    processedNode.push(neighbourNode);
                }
            }
        }

        if (order.size() != nodeCount())
        {
            m_lastError = "Cycle detected — topological sort failed";
            return {};
        }
        return order;
    };
    std::string lastError() { return m_lastError; }
    bool hasCycles()
    {
        std::unordered_set<NodeID> visited;
        std::unordered_set<NodeID> inStack;
        std::unordered_map<NodeID, NodeID> parentMap;

        for (auto &[node, _] : m_adjacentList)
        {
            if (!visited.count(node))
                if (dfsCycle(node, visited, inStack, parentMap))
                {
                    return true;
                }
        };

        return false;
    };
    std::vector<std::vector<NodeID>> findCycles()
    {
        std::unordered_set<NodeID> visited;
        std::unordered_set<NodeID> inStack;
        std::unordered_map<NodeID, NodeID> parentMap;
        std::vector<std::vector<NodeID>> cycles;

        for (auto &[node, _] : m_adjacentList)
        {
            if (!visited.count(node))
                dfsCycleCollect(node, visited, inStack, parentMap, cycles);
        };

        return cycles;
    }

    void print()
    {
        for (auto &[node, nbrs] : m_adjacentList)
        {
            std::string label = getLabel(node);
            std::cout << label << " [" << node << "]:";

            for (NodeID nbr : nbrs)
                std::cout << "  → " << getLabel(nbr)
                          << " [" << nbr << "]";
            std::cout << "\n";
        }
    }

    void printTopoOrder()
    {
        auto order = topoSort();

        if (order.empty())
        {
            std::cout << "[cycle detected — cannot produce topo order]\n";
            return;
        }

        for (size_t i = 0; i < order.size(); i++)
        {
            std::cout << (i + 1) << ". " << getLabel(order[i]);
            if (i + 1 < order.size())
                std::cout << "  →  ";
        }
        std::cout << "\n";
    }

    void printStats()
    {
        std::cout << "Nodes: " << nodeCount() << "\n";
        std::cout << "Edges: " << edgeCount() << "\n";
        std::cout << "Has Cycle: " << (hasCycles() ? "YES" : "No") << "\n";

        auto visited = bfs(m_adjacentList.begin()->first);

        std::cout << "Connected: "
                  << (visited.size() == nodeCount() ? "yes" : "NO") << "\n";
    }
    void printTree(NodeID root, int depth,
                   std::unordered_set<NodeID> &seen)
    {
        std::string indent(depth * 2, ' ');
        std::string label = getLabel(root);

        if (seen.count(root))
        {
            std::cout << indent << label << " (*) \n";
            return;
        }
        seen.insert(root);
        std::cout << indent << label << "\n";

        for (NodeID nbr : m_adjacentList.at(root))
            printTree(nbr, depth + 1, seen);
    }

private:
    bool dfsCycle(
        NodeID node,
        std::unordered_set<NodeID> &visited,
        std::unordered_set<NodeID> &inStack,
        std::unordered_map<NodeID, NodeID> &parentMap)
    {

        visited.insert(node);
        inStack.insert(node);

        for (auto &neighborNode : m_adjacentList.at(node))
        {

            if (!visited.count(neighborNode))
            {
                parentMap[neighborNode] = node;
                if (dfsCycle(neighborNode, visited, inStack, parentMap))
                {
                    return true;
                }
            }
            else if (inStack.count(neighborNode))
            {
                return true;
            }
        }

        inStack.erase(node);

        return false;
    }
    void dfsCycleCollect(
        NodeID node,
        std::unordered_set<NodeID> &visited,
        std::unordered_set<NodeID> &inStack,
        std::unordered_map<NodeID, NodeID> &parentMap,
        std::vector<std::vector<NodeID>> &cycles)
    {

        visited.insert(node);
        inStack.insert(node);

        for (auto &neighborNode : m_adjacentList.at(node))
        {

            if (!visited.count(neighborNode))
            {
                parentMap[neighborNode] = node;
                dfsCycleCollect(neighborNode, visited, inStack, parentMap, cycles);
            }
            else if (inStack.count(neighborNode))
            {
                std::vector<NodeID> cycle;
                NodeID currentNode = node;
                while (currentNode != neighborNode)
                {
                    cycle.push_back(currentNode);
                    currentNode = parentMap[currentNode];
                }

                cycle.push_back(neighborNode);
                cycle.push_back(node);
                std::reverse(cycle.begin(), cycle.end());
                cycles.push_back(cycle);
            }
        }

        inStack.erase(node);
    }
};