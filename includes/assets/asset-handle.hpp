#pragma once
#include "asset-types.hpp"
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <string>

using AssetID = uint32_t;
template <typename T>
struct AssetHandle
{
    AssetID id = 0;
    bool isValid() { return id != 0; }
    bool operator==(const AssetHandle &o) { return o.id == id; }
};

template <typename T>
struct AssetHandleHash
{
    size_t operator()(const AssetHandle<T> &o) const
    {
        return std::hash<AssetID>{}(o.id);
    }
    size_t operator()(const AssetID &id) const
    {
        return std::hash<AssetID>{}(id);
    }
};
using TextureHandle = AssetHandle<Texture>;
using ShaderHandle = AssetHandle<Shader>;
using MeshHandle = AssetHandle<Mesh>;

struct Material
{
    TextureHandle texture;
    ShaderHandle shader;
};
using MaterialHandle = AssetHandle<Material>;
