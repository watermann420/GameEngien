#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Minimal FBX placeholder loader with Blender-like unit scaling.
// Designed so Blender exports (unit scale 1.0, apply transforms) can drop in under EngineFiles/meshes.

struct MeshVertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct Material
{
    float baseColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA
    float metallic{ 0.0f };
    float roughness{ 0.5f };
    std::wstring albedoTexture;                  // path to texture (e.g., EngineFiles/textures/xxx.png)
};

class FbxModel
{
public:
    bool Load(const std::wstring& path, float scale = 1.0f);
    const std::vector<MeshVertex>& Vertices() const { return m_vertices; }
    const std::vector<uint32_t>& Indices() const { return m_indices; }
    const std::vector<Material>& Materials() const { return m_materials; }
    const std::vector<uint32_t>& MaterialIndices() const { return m_materialIndices; } // per-face material index
    float ImportScale() const { return m_importScale; }

private:
    std::vector<MeshVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<Material> m_materials;
    std::vector<uint32_t> m_materialIndices;
    float m_importScale{ 1.0f };
};
