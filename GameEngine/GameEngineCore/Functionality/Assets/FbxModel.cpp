#include "FbxModel.h"
#include <fstream>
#include <filesystem>

// Placeholder loader: currently only records the file path and scale.
// TODO: Integrate a real FBX parser (e.g., Assimp) and populate vertices/indices.

bool FbxModel::Load(const std::wstring& path, float scale)
{
    namespace fs = std::filesystem;
    if (!fs::exists(path)) return false;

    m_vertices.clear();
    m_indices.clear();
    m_importScale = scale;

    // Stub: fake a single quad to keep pipelines happy until a real loader is wired.
    MeshVertex v0{ -0.5f * scale, -0.5f * scale, 0, 0,0,1, 0,1 };
    MeshVertex v1{  0.5f * scale, -0.5f * scale, 0, 0,0,1, 1,1 };
    MeshVertex v2{  0.5f * scale,  0.5f * scale, 0, 0,0,1, 1,0 };
    MeshVertex v3{ -0.5f * scale,  0.5f * scale, 0, 0,0,1, 0,0 };
    m_vertices = { v0, v1, v2, v3 };
    m_indices = { 0,1,2, 0,2,3 };
    m_materials.clear();
    Material mat{};
    mat.albedoTexture = L""; // no texture yet
    m_materials.push_back(mat);
    m_materialIndices = { 0,0 }; // one material for the quad

    return true;
}
