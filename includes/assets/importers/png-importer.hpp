#pragma once
#include "../interface/Iimporter.hpp"
#include "../asset-types.hpp"
#include <stb_image.h>
#include <iostream>

struct PNGImporter : IImporter<Texture>
{
    std::optional<Texture> import(const std::filesystem::path &path) override
    {
        int w, h, c;
        uint8_t *raw = stbi_load(path.string().c_str(), &w, &h, &c, STBI_rgb_alpha);

        if (!raw)
        {
            std::cerr << "Failed to import " << path << "\n";
            return std::nullopt;
        }

        Texture tex;
        tex.width = w;
        tex.height = h;
        tex.channels = c;
        tex.pixels = std::vector<uint8_t>(raw, raw + w * h * 4);

        stbi_image_free(raw);

        return std::make_optional<Texture>(tex);
    };
};