#pragma once
#include "asset-handle.hpp"
#include <filesystem>
struct AssetMetadata
{
    AssetID id;
    AssetType assetType = AssetType::Unkown;
    std::string sourceFile;
    std::filesystem::file_time_type lastModified;
    std::string contentHash;
    std::string name;
};

class AssetIDGenerator
{
    static std::atomic<AssetID> nextId;

public:
    static AssetID generate() { return ++nextId; }
};

std::atomic<AssetID> AssetIDGenerator::nextId{0};

template <typename T>
class AssetRegistry
{
    std::unordered_map<AssetID, T, AssetHandleHash<T>> m_assets;
    std::unordered_set<AssetID> m_pendingAssets;

public:
    AssetHandle<T> registerAsset(T asset)
    {
        AssetID id = AssetIDGenerator::generate();

        m_assets.emplace(id, std::move(asset));
        return AssetHandle<T>{id};
    }

    T *get(AssetHandle<T> handle)
    {
        auto it = m_assets.find(handle.id);

        return it == m_assets.end() ? nullptr : &it->second;
    }

    bool hasAsset(AssetHandle<T> handle) const
    {
        return m_assets.count(handle.id) > 0;
    }

    void remove(AssetHandle<T> handle)
    {
        m_assets.erase(handle.id);
    }

    void replace(AssetHandle<T> handle, T asset)
    {
        remove(handle);
        m_assets.emplace(handle.id, std::move(asset));
    }

    void forEach(std::function<void(AssetID, T &)> fn)
    {
        for (auto &[id, asset] : m_assets)
        {
            fn(id, asset);
        }
    }

    void registerWithId(AssetID id, T asset)
    {
        m_assets.emplace(id, std::move(asset));
    }
    AssetHandle<T> reserve()
    {
        AssetID id = AssetIDGenerator::generate();
        m_pendingAssets.emplace(id);

        return AssetHandle<T>{id};
    }

    bool isPending(AssetHandle<T> handle)
    {
        return m_pendingAssets.count(handle.id) > 0;
    }

    void promote(AssetHandle<T> handle, T asset)
    {
        m_pendingAssets.erase(handle.id);
        m_assets.emplace(handle.id, std::move(asset));
    }
};
