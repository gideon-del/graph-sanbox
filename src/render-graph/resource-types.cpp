#include "render-graph/resource-types.hpp"

ResourceSize ResourceSize::bufferAbsolute(size_t bytes)
{
    ResourceSize size;

    size.type = ResourceSize::Type::Absolute;
    size.bytesPerTile = bytes;

    return size;
};

ResourceSize ResourceSize::bufferScreenRelative(size_t bytesPerPixel, float scale)
{
    ResourceSize size;

    size.type = ResourceSize::Type::ScreenRelative;
    size.bytesPerTile = bytesPerPixel;
    size.scale = scale;

    return size;
};
ResourceSize ResourceSize::bufferTileRelative(uint32_t tileSize, size_t bytesPerTile)
{
    ResourceSize size;

    size.type = ResourceSize::Type::TileRelative;
    size.bytesPerTile = bytesPerTile;
    size.tileSize = tileSize;

    return size;
};

ResourceSize ResourceSize::textureAbsolute(uint32_t w, uint32_t h)
{
    ResourceSize size;

    size.type = ResourceSize::Type::Absolute;
    size.width = std::max(w, 1u);
    size.height = std::max(h, 1u);

    return size;
};
ResourceSize ResourceSize::textureScreenRelative(float scale)
{
    ResourceSize size;

    size.type = ResourceSize::Type::ScreenRelative;
    size.scale = scale;

    return size;
};
uint32_t ResourceSize::calculateTextureWidth(const uint32_t screenWidth)
{
    switch (type)
    {
    case Type::ScreenRelative:
        return static_cast<uint32_t>(float(screenWidth) * scale);

    default:

        return std::max(width, 1u);
    }
}
uint32_t ResourceSize::calculateTextureHeight(const uint32_t screenHeight)
{
    switch (type)
    {
    case Type::ScreenRelative:
        return static_cast<uint32_t>(float(screenHeight) * scale);

    default:

        return std::max(height, 1u);
    }
}
size_t ResourceSize::calculateBufferSize(const uint32_t screenWidth, const uint32_t screenHeight)
{
    switch (type)
    {
    case Type::ScreenRelative:
    {
        uint32_t w = uint32_t(screenWidth * scale);
        uint32_t h = uint32_t(screenHeight * scale);
        return std::max(size_t(1), size_t(w) * h * bytesPerTile);
    }
    case Type::TileRelative:
    {

        uint32_t tilesX = (screenWidth + tileSize - 1) / tileSize;
        uint32_t tilesY = (screenHeight + tileSize - 1) / tileSize;
        return std::max(size_t(1), size_t(tilesX) * tilesY * bytesPerTile);
    }

    default:

        return std::max(bytesPerTile, size_t(1));
    }
}