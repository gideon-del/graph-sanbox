#pragma once
#include "../interface/Iimporter.hpp"
#include "../asset-types.hpp"
#include <tiny_obj_loader.h>

struct OBJImporter : IImporter<Mesh>
{
    std::optional<Mesh> import(const std::filesystem::path &path) override
    {
        tinyobj::attrib_t attrib;

        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        bool success = tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            path.string().c_str());

        if (!warn.empty())
        {
            std::cout << "Warning from tiny obj: " + warn << std::endl;
        }
        if (!err.empty())
        {
            std::cerr << "Error from tiny obj: " + err << std::endl;
        }

        if (!success)
        {
            std::cerr << "Failed to load " << path << "\n";
            return std::nullopt;
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        for (const auto &shape : shapes)
        {

            for (const auto &index : shape.mesh.indices)
            {
                Vertex vertex;

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
                };

                if (index.normal_index >= 0)
                {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                    };
                }
                if (index.texcoord_index >= 0)
                {
                    vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1],

                    };
                }

                vertices.push_back(vertex);
                indices.push_back(static_cast<uint32_t>(indices.size()));
            }
        }
        Mesh mesh;
        mesh.vertices = vertices;
        mesh.indices = indices;

        return std::make_optional<Mesh>(mesh);
    };
};