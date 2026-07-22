#pragma once
#include <optional>
#include <filesystem>
#include <iostream>

template <typename T>
struct IImporter
{

    virtual std::optional<T> import(const std::filesystem::path &path) = 0;
    virtual ~IImporter() = default;
};
