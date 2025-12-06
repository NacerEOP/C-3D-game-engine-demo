#include "ManualMesh.hpp"

ManualMesh::ManualMesh(const std::vector<Vertex>& vertices, 
                       const std::vector<unsigned int>& indices,
                       const PBRMaterial& material)
    : Mesh(vertices, indices, {}, material) {
}


