#pragma once

#import <vector>

struct Vertex
{
    float vx, vy, vz;
    float nx, ny, nz;
    float tu, tv;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

bool loadMesh(Mesh& result, const char* path);


