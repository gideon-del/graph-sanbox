#include "render-graph/resource-manager.hpp"
#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <string>
#include <format>
// bool heapRecreated(HeapId id, const std::vector<AliasCommand> &commands)
// {
//     return std::any_of(commands.begin(), commands.end(), [id](const AliasCommand &c)
//                        {
//         auto* create = std::get_if<CreateHeapCommand>(&c);
//         return create && create->id == id; });
// }

const std::vector<AliasCommand> ResourceManager::buildTransientResources(uint32_t frameWidth, uint32_t frameHeight, const PhysicalTextureLifetime &lifetimes)
{
    auto newPlan = calculateAliasPlan(lifetimes, frameWidth, frameHeight);
    auto commands = diffAliasPlan(newPlan);

    for (auto &command : commands)
    {
        std::visit(
            [this](const auto &c)
            {
                applyAlias(c);
            },
            command);
    }
    for (auto &[id, group] : newPlan.groups)
        group.requirement = m_activeHeaps.at(id);
    m_currentAliasPlan = newPlan;
    m_lastLifetime = lifetimes;

    return commands;
}

AliasPlan ResourceManager::calculateAliasPlan(const PhysicalTextureLifetime &lifetimes, const uint32_t width, const uint32_t height)
{
    struct ResourceAllocEntry
    {
        TextureHandle handle;
        ResourceLifetimeV2 lifetime;
        size_t sizeRequired;
    };

    std::vector<ResourceAllocEntry> entries;

    for (auto &[handle, lifetime] : lifetimes)
    {
        if (!isValid(handle) || (m_textures[handle.id].lifetime != ResourceLifetimeType::Transient || m_textures[handle.id].instanceType != ResourceInstanceType::Single))
            continue;
        auto &textureDesc = m_textures[handle.id].descriptor;
        size_t byteForFormat = 4;
        size_t mockSize = (size_t)textureDesc.size.calculateTextureWidth(width) * (size_t)textureDesc.size.calculateTextureHeight(height) * byteForFormat;
        entries.push_back({handle, lifetime, mockSize});
    }

    std::sort(entries.begin(), entries.end(), [](const ResourceAllocEntry &a, const ResourceAllocEntry &b)
              { return a.lifetime.createdAt < b.lifetime.createdAt; });

    AliasPlan plan;

    HeapId nextHeapId = 0;

    std::unordered_map<HeapId, std::vector<std::pair<int, int>>> heapActiveIntervals;

    for (const auto &entry : entries)
    {
        bool placed = false;
        auto &desc = m_textures[entry.handle.id].descriptor;

        Extent2D extent{desc.size.calculateTextureWidth(width),
                        desc.size.calculateTextureHeight(height)};
        plan.textureExtents[entry.handle] = extent;
        for (auto &[id, intervals] : heapActiveIntervals)
        {
            bool overlaps = false;

            for (auto &interval : intervals)
            {
                if (entry.lifetime.createdAt <= interval.second && entry.lifetime.lastUse >= interval.first)
                {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps)
            {
                auto it = plan.groups.find(id);
                if (it != plan.groups.end())
                {
                    it->second.textures.push_back(entry.handle);
                    it->second.requirement.size = std::max(it->second.requirement.size, entry.sizeRequired);
                    intervals.push_back({entry.lifetime.createdAt, entry.lifetime.lastUse});
                    plan.textureToHeap[entry.handle] = id;
                    placed = true;
                    break;
                };
            }
        }

        if (!placed)
        {
            HeapId newId = nextHeapId++;
            AliasGroup newGroup;

            newGroup.id = newId;
            newGroup.requirement.size = entry.sizeRequired;
            newGroup.requirement.memorytypeIndex = 0;
            newGroup.textures.push_back(entry.handle);

            plan.groups[newId] = newGroup;
            plan.textureToHeap[entry.handle] = newId;
            heapActiveIntervals[newId].push_back({entry.lifetime.createdAt, entry.lifetime.lastUse});
        }
    }
    return plan;
};

std::vector<AliasCommand> ResourceManager::diffAliasPlan(const AliasPlan &newPlan)
{
    std::vector<AliasCommand> commands;

    std::unordered_set<HeapId> targetedHeaps;
    std::unordered_set<TextureHandle, TextureHandleHash> targetedTextures;
    auto heapExists = [&](HeapId id)
    {
        return m_currentAliasPlan.groups.contains(id);
    };
    for (auto &[id, group] : newPlan.groups)
    {
        targetedHeaps.insert(group.id);
        bool heapChanged = false;
        if (!heapExists(group.id))
        {
            commands.push_back(CreateHeapCommand{group.id, group.requirement});
            heapChanged = true;
        }
        else
        {
            auto &heap = m_currentAliasPlan.groups.at(group.id);
            if (heap.requirement.size < group.requirement.size)
            {
                commands.push_back(DeleteHeapCommand{group.id});
                commands.push_back(CreateHeapCommand{group.id, group.requirement});
                heapChanged = true;
            }
        }

        for (auto &texture : group.textures)
        {
            targetedTextures.insert(texture);

            auto currentHeap = m_currentAliasPlan.textureToHeap.find(texture);
            bool existed = currentHeap != m_currentAliasPlan.textureToHeap.end();
            bool moved = existed && currentHeap->second != group.id;
            bool resized = existed &&
                           m_currentAliasPlan.textureExtents.at(texture) != newPlan.textureExtents.at(texture);
            if (!existed || moved || heapChanged || resized)
            {
                if (existed)
                    commands.push_back(DeleteAliasTextureCommand{texture});
                commands.push_back(CreateAliasTextureCommand{group.id, texture, newPlan.textureExtents.at(texture)});
            }
        }
    }

    for (auto it = m_currentAliasPlan.textureToHeap.begin(); it != m_currentAliasPlan.textureToHeap.end();)
    {

        if (targetedTextures.find(it->first) == targetedTextures.end())
        {
            commands.push_back(DeleteAliasTextureCommand{it->first});

            // it = m_currentAliasPlan.textureToHeap.erase(it);
        }
        ++it;
    }

    for (auto &[id, heap] : m_currentAliasPlan.groups)
    {
        if (targetedHeaps.find(id) == targetedHeaps.end())
        {
            commands.push_back(DeleteHeapCommand{id});
        }
    }

    auto phase = [](const AliasCommand &c)
    {
        return std::visit([](const auto &x) -> int
                          {
        using T = std::decay_t<decltype(x)>;

       if constexpr (std::is_same_v<T, DeleteAliasTextureCommand>)
   { return 0;}
else if constexpr (std::is_same_v<T, DeleteHeapCommand>)
  {  return 1;}
else if constexpr (std::is_same_v<T, CreateHeapCommand>)
    {return 2;}
else
   { return 3;} }, c);
    };

    std::stable_sort(commands.begin(), commands.end(), [&](const auto &a, const auto &b)
                     { return phase(a) < phase(b); });

    return commands;
}

void ResourceManager::applyAlias(const CreateHeapCommand &command)
{

    // In a real use case I'll call the rhi to create the heap and return the heap handle

    m_activeHeaps[command.id] = command.requirement;
}
void ResourceManager::applyAlias(const CreateAliasTextureCommand &command)
{

    // For this i just use that heap to create the aliased texture based on the texture requirement
    if (!isValid(command.texture))
    {
        return;
    }
    if (!m_activeHeaps.contains(command.id))
    {
        throw std::runtime_error(std::format("No Heap found with id {}", command.id));
    }

    auto &resource = m_textures[command.texture.id];

    resource.resources.create(resource.instanceCount,
                              [&](uint32_t frameIdx)
                              {
                                  return GPUTexture{
                                      command.extent, TextureState(resource.descriptor.mipCount, 1)};
                              });
    //   Also i might need to pass frame width and height  in the case of screen relative textures
}

void ResourceManager::applyAlias(const DeleteAliasTextureCommand &command)
{
    if (!isValid(command.texture))
        return;
    auto &resource = m_textures[command.texture.id];

    resource.resources.clear();
}

void ResourceManager::applyAlias(const DeleteHeapCommand &command)
{
    m_activeHeaps.erase(command.id);
}

const std::vector<AliasCommand> ResourceManager::resize(uint32_t width, uint32_t height)
{
    for (auto &h : m_screenRelativeTextures)
    {
        if (m_currentAliasPlan.textureToHeap.contains(h))
            continue;
        auto &tex = m_textures[h.id];
        tex.resources.clear();
        tex.resources.create(tex.instanceCount, [&](uint32_t)
                             { return GPUTexture{
                                   .extent = {tex.descriptor.size.calculateTextureWidth(width), tex.descriptor.size.calculateTextureHeight(height)}}; });
    }

    for (auto &h : m_screenRelativeBuffers)
    {
        auto &buf = m_buffers[h.id];
        buf.resources.clear();
        buf.resources.create(buf.instanceCount, [](uint32_t)
                             { return GPUBuffer{}; });
    }

    auto commands = buildTransientResources(width, height, m_lastLifetime);

    m_historyValid = false;

    return commands;
}

void ResourceManager::transition(TextureState &state, const SubResourceRange &range, ResourceState to, std::vector<TextureAccess> &batch, TextureHandle handle, bool history)
{
    bool wholeTexture = range.isAll();
    bool uniform = state.isUniform();

    auto resolvedRange = range.resolve(state.mipCount(), state.layerCount());

    if (wholeTexture && uniform)
    {
        auto currentState = state.get(0);

        if (currentState == to)
            return;
        resolvedRange.from = currentState;
        batch.push_back({

            .resource = handle, // In the renderer this will hold either the previous or current handle to the renderer based on the history
            .state = to,
            .range = resolvedRange,
            .history = history

        });

        state.updateAll(to);

        return;
    }

    for (uint32_t layer = resolvedRange.baseLayer; layer < resolvedRange.baseLayer + resolvedRange.layerCount; ++layer)
    {

        uint32_t runStart = resolvedRange.baseMip;
        ResourceState runState = state.get(state.subIndex(runStart, layer));

        auto flush = [&](uint32_t end)
        {
            if (runState != to)
            {

                SubResourceRange newRange{};
                newRange.baseMip = runStart;
                newRange.baseLayer = layer;
                newRange.mipCount = end - runStart;
                newRange.layerCount = 1;
                newRange.from = runState;
                batch.push_back({handle, to, newRange, history});
            }
        };

        for (uint32_t mip = resolvedRange.baseMip; mip < resolvedRange.baseMip + resolvedRange.mipCount; mip++)
        {
            auto subState = state.get(state.subIndex(mip, layer));

            if (subState != runState)
            {
                flush(mip);
                runState = subState;
                runStart = mip;
            }
        }

        flush(resolvedRange.baseMip + resolvedRange.mipCount);
    }
    state.updateRange(to, resolvedRange.baseMip, resolvedRange.mipCount, resolvedRange.baseLayer, resolvedRange.layerCount);
    // auto splitMipBarrier = [&]()
    // {
    //     uint32_t maxMip = resolvedRange.baseMip + resolvedRange.mipCount;
    //     uint32_t maxLayer = resolvedRange.baseLayer + resolvedRange.layerCount;

    //     assert(maxMip <= state.mipCount());
    //     assert(maxLayer <= state.layerCount());
    //     SubResourceRange currentResourceRange{};
    //     currentResourceRange.baseMip = resolvedRange.baseMip;
    //     currentResourceRange.baseLayer = resolvedRange.baseLayer;
    //     currentResourceRange.mipCount = 1;
    //     currentResourceRange.layerCount = 1;

    //     ResourceState currentGroupState = state.get(currentResourceRange.baseMip * currentResourceRange.baseLayer);

    //     for (uint32_t mip = resolvedRange.baseMip; mip < maxMip; mip++)
    //     {
    //         for (uint32_t layer = resolvedRange.baseLayer; layer < maxLayer; layer++)
    //         {
    //             auto currentSubState = state.get(state.subIndex(mip, layer));

    //             if (currentSubState == currentGroupState)
    //             {
    //                 currentResourceRange.mipCount = std::max(mip - currentResourceRange.baseMip, 1u);
    //                 currentResourceRange.layerCount = std::max(layer - currentResourceRange.baseLayer, 1u);
    //                 continue;
    //             };

    //             if (currentGroupState != to)
    //             {

    //                 currentResourceRange.from = currentGroupState;
    //                 batch.push_back({

    //                     .resource = handle,
    //                     .state = to,
    //                     .range = currentResourceRange,
    //                     .history = history

    //                 });

    //                 state.updateRange(
    //                     to,
    //                     currentResourceRange.baseMip,
    //                     currentResourceRange.mipCount,
    //                     currentResourceRange.baseLayer,
    //                     currentResourceRange.layerCount);
    //             }

    //             currentResourceRange.baseMip = mip;
    //             currentResourceRange.baseLayer = layer;
    //             currentResourceRange.mipCount = 1;
    //             currentResourceRange.layerCount = 1;

    //             currentGroupState = currentSubState;
    //         }
    //     }

    //     if (currentGroupState != to)
    //     {
    //         currentResourceRange.from = currentGroupState;
    //         batch.push_back({

    //             .resource = handle,
    //             .state = to,
    //             .range = currentResourceRange,
    //             .history = history

    //         });

    //         state.updateRange(
    //             to,
    //             currentResourceRange.baseMip,
    //             currentResourceRange.mipCount,
    //             currentResourceRange.baseLayer,
    //             currentResourceRange.layerCount);
    //     }
    // };

    // splitMipBarrier();
};