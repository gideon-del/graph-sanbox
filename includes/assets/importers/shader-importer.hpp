#pragma once
#include "../interface/Iimporter.hpp"
#include "../asset-types.hpp"
#include <fstream>

struct ShaderImporter : IImporter<Shader>
{
    std::optional<Shader> import(const std::filesystem::path &path) override
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            std::cerr << "Failed to import " << path << "\n";
            return std::nullopt;
        }

        size_t size = file.tellg();
        file.seekg(0);

        Shader shader;
        shader.bytecode.resize(size);
        file.read(reinterpret_cast<char *>(shader.bytecode.data()), size);

        return std::make_optional<Shader>(shader);
    }
};