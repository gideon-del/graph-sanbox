#pragma once
#include "asset-handle.hpp"
#include "./importers/importers.hpp"
#include "../graph.hpp"
#include <picosha2.h>

enum class AssetType
{
    Unkown,
    Texture,
    Material,
    Mesh,
    Shader
};
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
};

class AssetManager
{
public:
    std::string computeSha256Hash(const std::filesystem::path &path)
    {

        std::ifstream file(path, std::ios::binary);

        std::vector<char> bytes(std::istreambuf_iterator<char>(file), {});
        return picosha2::hash256_hex_string(bytes);
    }
    TextureHandle importTexture(
        const std::filesystem::path &path)
    {
        PNGImporter importer;

        auto texture = importer.import(path);

        if (!texture)
        {
            return AssetHandle<Texture>{};
        }

        TextureHandle handle = textures.registerAsset(std::move(*texture));

        AssetMetadata metadata;
        metadata.id = handle.id;
        metadata.sourceFile = path.string();
        metadata.lastModified = std::filesystem::last_write_time(path);
        metadata.assetType = AssetType::Texture;
        metadata.name = path.filename().string();
        metadata.contentHash = computeSha256Hash(path);

        _metadata[handle.id] = metadata;
        _depGraph.addNode(handle.id, metadata.name);
        return handle;
    }

    MeshHandle importMesh(const std::filesystem::path &path)
    {
        OBJImporter importer;
        auto mesh = importer.import(path);
        if (!mesh)
        {
            return AssetHandle<Mesh>{};
        }

        MeshHandle handle = meshes.registerAsset(std::move(*mesh));

        AssetMetadata metadata;
        metadata.id = handle.id;
        metadata.sourceFile = path.string();
        metadata.lastModified = std::filesystem::last_write_time(path);
        metadata.assetType = AssetType::Mesh;
        metadata.name = path.filename().string();
        metadata.contentHash = computeSha256Hash(path);

        _metadata[handle.id] = metadata;
        _depGraph.addNode(handle.id, metadata.name);

        return handle;
    }

    ShaderHandle importShader(const std::filesystem::path &path)
    {
        ShaderImporter importer;
        auto shader = importer.import(path);
        if (!shader)
        {
            return AssetHandle<Shader>{};
        }

        ShaderHandle handle = shaders.registerAsset(std::move(*shader));

        AssetMetadata metadata;
        metadata.id = handle.id;
        metadata.sourceFile = path.string();
        metadata.lastModified = std::filesystem::last_write_time(path);

        metadata.assetType = AssetType::Shader;
        metadata.name = path.filename().string();

        metadata.contentHash = computeSha256Hash(path);

        _metadata[handle.id] = metadata;
        _depGraph.addNode(handle.id, metadata.name);

        return handle;
    }

    MaterialHandle importMaterial(const std::filesystem::path &texturePath, const std::filesystem::path &shaderPath)
    {
        auto texture = importTexture(texturePath);
        auto shader = importShader(shaderPath);

        if (!texture.isValid() || !shader.isValid())
        {
            return AssetHandle<Material>();
        }
        MaterialHandle materialHandle = materials.registerAsset(Material{
            .texture = texture,
            .shader = shader});

        _depGraph.addNode(materialHandle.id, "Material " + std::to_string(materialHandle.id));
        try
        {
            addDependency(materialHandle.id, texture.id);
            addDependency(materialHandle.id, shader.id);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            _depGraph.removeNode(materialHandle.id);
            materials.remove(materialHandle);
            return AssetHandle<Material>();
        }

        return materialHandle;
    }

    std::string assetName(AssetID id)
    {
        return _metadata[id].name;
    }

    void printDependencies()
    {
        _depGraph.printStats();
        std::cout << "Load order: ";
        _depGraph.printTopoOrder();
    }

    std::unordered_set<AssetID> invalidate(AssetID id)
    {
        assert(_depGraph.hasNode(id));
        auto affectedAssets = _depGraph.bfs(id);
        return std::unordered_set<AssetID>(affectedAssets.begin(), affectedAssets.end());
    }

    void checkForChanges()
    {
        for (auto &[id, meta] : _metadata)
        {
            if (meta.sourceFile.empty())
                continue;

            std::string currentHash = computeSha256Hash(meta.sourceFile);
            if (currentHash != meta.contentHash)
            {
                std::cout << "Changed: " << meta.sourceFile << "\n";
                meta.contentHash = currentHash;

                auto affected = invalidate(id);
                auto order = _depGraph.topoSort();
                for (auto &aid : order)
                {
                    if (affected.count(aid))
                        reload(aid);
                }
            }
        }
    }

    void reload(AssetID id)
    {
        assert(_depGraph.hasNode(id));
        if (!_metadata.count(id))
            return;
        auto &meta = _metadata.at(id);

        if (meta.assetType == AssetType::Texture)
            reloadTexture(id, meta);
        if (meta.assetType == AssetType::Mesh)
            reloadMesh(id, meta);
        if (meta.assetType == AssetType::Shader)
            reloadShader(id, meta);
    }

    AssetRegistry<Texture> textures;
    AssetRegistry<Shader> shaders;
    AssetRegistry<Mesh> meshes;
    AssetRegistry<Material> materials;

private:
    std::unordered_map<AssetID, AssetMetadata> _metadata;
    Graph _depGraph;
    void reloadTexture(AssetID id, AssetMetadata &meta)
    {
        TextureHandle handle = AssetHandle<Texture>{id};
        PNGImporter importer;
        auto texture = importer.import(meta.sourceFile);
        if (!texture)
        {
            std::cout << "Failed to reload texture at: " << meta.sourceFile << "\n";
            return;
        }
        textures.replace(handle, std::move(*texture));
    };
    void reloadMesh(AssetID id, AssetMetadata &meta)
    {
        MeshHandle handle = AssetHandle<Mesh>{id};
        OBJImporter importer;
        auto mesh = importer.import(meta.sourceFile);
        if (!mesh)
        {
            std::cout << "Failed to reload mesh at: " << meta.sourceFile << "\n";
            return;
        }
        meshes.replace(handle, std::move(*mesh));
    };
    void reloadShader(AssetID id, AssetMetadata &meta)
    {
        ShaderHandle handle = AssetHandle<Shader>{id};
        ShaderImporter importer;
        auto shader = importer.import(meta.sourceFile);
        if (!shader)
        {
            std::cout << "Failed to reload shader at: " << meta.sourceFile << "\n";
            return;
        }
        shaders.replace(handle, std::move(*shader));
    };
    void addDependency(AssetID from, AssetID to)
    {
        _depGraph.addEdge(to, from);
        auto cycles = _depGraph.findCycles();
        if (!cycles.empty())
        {
            _depGraph.removeEdge(to, from);

            for (auto &cycle : cycles)
            {
                for (size_t i = 0; i < cycle.size(); i++)
                {
                    std::cerr << assetName(cycle[i]);
                    if (i + 1 < cycle.size())
                        std::cerr << " → ";
                }
                std::cerr << "\n";
            }
            throw std::runtime_error("Circular asset dependency");
        }
    }
};