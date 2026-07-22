#pragma once
#include "asset-handle.hpp"
#include "./importers/importers.hpp"
#include "../graph.hpp"

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

struct AssetManager
{
    AssetRegistry<Texture> textures;

    AssetRegistry<Shader> shaders;

    AssetRegistry<Mesh> meshes;

    AssetRegistry<Material> materials;

    std::unordered_map<AssetID, AssetMetadata> _metadata;
    Graph _depGraph;
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

        _metadata[handle.id] = metadata;
        _depGraph.addNode(handle.id);
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
        _metadata[handle.id] = metadata;
        _depGraph.addNode(handle.id);

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

        _metadata[handle.id] = metadata;
        _depGraph.addNode(handle.id);

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

        _depGraph.addNode(materialHandle.id);
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

    void printDependencies()
    {
        _depGraph.printStats();
        std::cout << "Load order: ";
        _depGraph.printTopoOrder();
    }
};