#pragma once

#import <vector>

struct Vertex
{
    float vx, vy, vz, vw;
    float nx, ny, nz,nw;
    float tu, tv, pad1,pad2;
};

struct MeshObj
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

bool loadMeshObj(MeshObj& result, const char* path);


