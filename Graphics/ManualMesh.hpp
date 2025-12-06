#pragma once
#include "Mesh.hpp"
#include <vector>

class ManualMesh : public Mesh {
public:
    ManualMesh(const std::vector<Vertex>& vertices, 
               const std::vector<unsigned int>& indices,
               const PBRMaterial& material);
    ~ManualMesh() = default;

    
};