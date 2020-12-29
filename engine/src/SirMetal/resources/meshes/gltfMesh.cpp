#include "SirMetal/resources/meshes/gltfMesh.h"
#include "SirMetal/resources/gltfLoader.h"
#include "SirMetal/resources/meshes/meshOptimize.h"

#include <cgltf/cgltf.h>
#include <xatlas/xatlas.h>

#include <chrono>

namespace SirMetal {
static cgltf_size component_size(cgltf_component_type component_type) {
  switch (component_type) {
    case cgltf_component_type_r_8:
    case cgltf_component_type_r_8u:
      return 1;
    case cgltf_component_type_r_16:
    case cgltf_component_type_r_16u:
      return 2;
    case cgltf_component_type_r_32u:
    case cgltf_component_type_r_32f:
      return 4;
    case cgltf_component_type_invalid:
    default:
      return 0;
  }
}

static cgltf_size component_count(cgltf_type type) {
  switch (type) {
    case cgltf_type_scalar:
      return 1;
    case cgltf_type_vec2:
      return 2;
    case cgltf_type_vec3:
      return 3;
    case cgltf_type_vec4:
    case cgltf_type_mat2:
      return 4;
    case cgltf_type_mat3:
      return 9;
    case cgltf_type_mat4:
      return 16;
    case cgltf_type_invalid:
      assert(0 && "invalid cgltf type size");
      return 0;
  }
}

bool loadGltfMesh(MeshLoadResult &outMesh, const void *gltfMesh, uint32_t flags) {
  const auto *mesh = reinterpret_cast<const cgltf_mesh *>(gltfMesh);

  auto gltfFlags = static_cast<GLTFLoadFlags>(flags);
  // assuming primitive count 1
  assert(mesh->primitives_count == 1);
  cgltf_primitive &prim = mesh->primitives[0];

  int uniqueVerticesCount = -1;
  // we can't simply throw memory into a buffer and hope it works. we are going
  // to create views for the same buffer, each view will point to a different
  // part of the buffer, for example a view will only refer to the position of
  // the mesh and so on.As such the data needs to be correctly aligned for
  // example normals appear after positions, the start of the normals data needs
  // to be aligned to the requested boundary if this is not the case we will add
  // some padding.
  constexpr uint32_t aligmentInFloats = 64;

  // here we accumulate all the extra data we added due to alignment
  int aligmentOffsets = 0;

  // for our mesh to be normalized to what the engine expects we require 4
  // attributes pos,normals, uvs and tangents, we look for such attributes in
  // the Gltf
  bool found = false;
  int foundIdx = 0;

  std::vector<float> fullMeshData[MESH_ATTRIBUTE_TYPE_COUNT];
  float strides[MESH_ATTRIBUTE_TYPE_COUNT] = {};

  for (int attrIdx = 0; attrIdx < MESH_ATTRIBUTE_TYPE_COUNT; ++attrIdx) {
    const char *attribute = MESH_ATTRIBUTES[attrIdx];
    for (int a = 0; a < prim.attributes_count; ++a) {
      const cgltf_attribute &primAttr = prim.attributes[a];
      found = strcmp(primAttr.name, attribute) == 0;
      if (found) {
        foundIdx = a;
        break;
      }
    }

    // if attribute is not found we patch in dummy data
    if (!found) {
      bool required = MESH_ATTRIBUTE_REQUIRED[attrIdx];

      // we just make sure we know how many unique vertices we have
      // this should never trigger because if we don't have position is useless
      // data
      if (strcmp(attribute, MESH_ATTRIBUTES[MESH_ATTRIBUTE_TYPE_POSITION]) == 0) {
        printf("[ERROR] Position mesh attribute is empty, cannot be filled "
               "with zeroes");
        return {};
      }

      //if the attribute is required we fill it with zero and print a warning
      //if not we simply continue
      if (required) {
        printf("[ERROR] Could not find %s attribute in gltf file, and is a required one"
               "... filling with zeroes\n",
               MESH_ATTRIBUTES[attrIdx]);
        assert(uniqueVerticesCount != -1);
        uint32_t sizeInFloats = MESH_ATTRIBUTE_SIZE_IN_BYTES[attrIdx] / sizeof(float);
        fullMeshData[attrIdx].resize(uniqueVerticesCount * sizeInFloats);
        memset(fullMeshData[attrIdx].data(), 0,
               fullMeshData[attrIdx].size() * sizeof(float));
      }
      continue;
    }

    assert(foundIdx < 4);
    auto &meshData = fullMeshData[attrIdx];

    // gltf maps a vertex attribute to an accessor, an accessor contains data
    // telling us how to read/access the data to make sense of it.
    const cgltf_accessor *accessor = mesh->primitives->attributes[foundIdx].data;
    // the buffer view will tell us which region of the buffer contains the data
    // we need
    const cgltf_buffer_view *view = accessor->buffer_view;

    // making sure all the vertex attributes have
    int accessorCount = static_cast<int>(accessor->count);
    uniqueVerticesCount = uniqueVerticesCount == -1 ? accessorCount : uniqueVerticesCount;
    if (uniqueVerticesCount != accessorCount) {
      printf("[ERROR] Mismatched mesh attribute count for index %i. Required "
             "%i but got %i\n",
             attrIdx, uniqueVerticesCount, accessorCount);
      return {};
    }

    const float *dataToCopy =
            reinterpret_cast<float *>((char *) view->buffer->data + view->offset);

    // this tell us the component size in bytes, so if we have a vec3, it means
    // we have 3 components, each component will have size 4bytes
    int datatypeSize = component_size(accessor->component_type);
    if (datatypeSize != 4) {
      printf("Vertex attribute with index %i has unexpected component width. "
             "Expected 4, got %i\n",
             attrIdx, datatypeSize);
      return {};
    }
    int componentCount = component_count(accessor->type);
    strides[attrIdx] = MESH_ATTRIBUTE_SIZE_IN_BYTES[attrIdx] / 4;

    // we are normalizing the data to float4 or float2, this means we will need
    // to fill a value if we get a vec3, we use a different filler per attribute
    float filler = attrIdx == 0 ? 1.0f : 0.0f;

    for (int idx = 0; idx < uniqueVerticesCount; ++idx) {
      meshData.push_back(dataToCopy[idx * componentCount + 0]);
      meshData.push_back(dataToCopy[idx * componentCount + 1]);
      if (attrIdx != MESH_ATTRIBUTE_TYPE_UV) {
        // if is not a UV (float2) we push in the 3rd parameter plus filler
        meshData.push_back(dataToCopy[idx * componentCount + 2]);
        meshData.push_back(filler);
      }
    }

    size_t end = static_cast<uint32_t>(meshData.size());

    if (attrIdx == MESH_ATTRIBUTE_TYPE_POSITION) {
      // generate bounding boxes for the positions
      float minX = std::numeric_limits<float>::max();
      float minY = minX;
      float minZ = minY;
      float maxX = std::numeric_limits<float>::min();
      float maxY = maxX;
      float maxZ = maxX;
      const float *posData = meshData.data();
      for (size_t p = 0; p < end; p += 4) {
        const float *vtx = posData + p;

        // lets us compute bounding box
        minX = vtx[0] < minX ? vtx[0] : minX;
        minY = vtx[1] < minY ? vtx[1] : minY;
        minZ = vtx[2] < minZ ? vtx[2] : minZ;

        maxX = vtx[0] > maxX ? vtx[0] : maxX;
        maxY = vtx[1] > maxY ? vtx[1] : maxY;
        maxZ = vtx[2] > maxZ ? vtx[2] : maxZ;
      }
      outMesh.m_boundingBox[0] = minX;
      outMesh.m_boundingBox[1] = minY;
      outMesh.m_boundingBox[2] = minZ;
      outMesh.m_boundingBox[3] = maxX;
      outMesh.m_boundingBox[4] = maxY;
      outMesh.m_boundingBox[5] = maxZ;
    }
  }

  // processing the index buffer
  const cgltf_accessor *accessor = prim.indices;
  const cgltf_buffer_view *view = accessor->buffer_view;
  cgltf_size componentSize = component_size(accessor->component_type);

  if (componentSize == 4) {
    const auto *source = reinterpret_cast<uint32_t const *>((char *) view->buffer->data +
                                                            view->offset);
    for (size_t idx = 0; idx < accessor->count; ++idx) {
      outMesh.indices.push_back(source[idx]);
    }
  } else {
    if (componentSize != 2) {
      printf("Mesh index buffer needs to be either 4 or 2 bytes, got %lu\n", componentSize);
      return {};
    }
    const auto *source = reinterpret_cast<uint16_t const *>((char *) view->buffer->data +
                                                            view->offset);
    for (size_t idx = 0; idx < accessor->count; ++idx) {
      outMesh.indices.push_back(source[idx]);
    }
  }

  bool generateLightUVs = (gltfFlags & GLTF_LOAD_FLAGS_GENERATE_LIGHT_MAP_UVS) > 0;
  if (generateLightUVs) {
    auto t1 = std::chrono::high_resolution_clock::now();

    //let us generate the uvs for lightmapping
    //Atlas_Dim dim;
    xatlas::Atlas *atlas = xatlas::Create();
    // Prepare mesh to be processed by xatlas:
    {
      xatlas::MeshDecl meshDcl;
      meshDcl.vertexCount = static_cast<uint32_t>(fullMeshData[0].size()) / 4;
      meshDcl.vertexPositionData = fullMeshData[0].data();
      meshDcl.vertexPositionStride = sizeof(float) * 4;
      meshDcl.vertexNormalData = fullMeshData[1].data();
      meshDcl.vertexNormalStride = sizeof(float) * 4;
      meshDcl.vertexUvData = fullMeshData[2].data();
      meshDcl.vertexUvStride = sizeof(float) * 2;
      meshDcl.indexCount = outMesh.indices.size();
      meshDcl.indexData = outMesh.indices.data();
      meshDcl.indexFormat = xatlas::IndexFormat::UInt32;
      xatlas::AddMeshError::Enum error = xatlas::AddMesh(atlas, meshDcl);
      if (error != xatlas::AddMeshError::Success) {
        printf("error adding atlas");
        return false;
      }
    }

    // Generate atlas:
    {
      xatlas::ChartOptions chartoptions;
      xatlas::PackOptions packoptions;

      packoptions.resolution = 2048;
      packoptions.blockAlign = true;

      xatlas::Generate(atlas, chartoptions, packoptions);
      int aw = atlas->width;
      int ah = atlas->height;


      xatlas::Mesh &atlasMesh = atlas->meshes[0];

      outMesh.indices.clear();
      outMesh.indices.resize(atlasMesh.indexCount);
      std::vector<float> apos(atlasMesh.vertexCount * 4);
      std::vector<float> anorm(atlasMesh.vertexCount * 4);
      std::vector<float> auv(atlasMesh.vertexCount * 2);
      std::vector<float> atan(atlasMesh.vertexCount * 4);
      fullMeshData[MESH_ATTRIBUTE_TYPE_UV_LIGHTMAP].resize(atlasMesh.vertexCount * 2);


      for (uint32_t j = 0; j < atlasMesh.indexCount; ++j) {
        const uint32_t ind = atlasMesh.indexArray[j];
        const xatlas::Vertex &v = atlasMesh.vertexArray[ind];
        assert(j < outMesh.indices.size());
        outMesh.indices[j] = ind;
        uint32_t vid = ind * 4u;
        uint32_t vidSrc = v.xref * 4u;
        assert(vid < apos.size());
        assert(vidSrc < fullMeshData[0].size());
        apos[vid + 0] = fullMeshData[0][vidSrc + 0];
        apos[vid + 1] = fullMeshData[0][vidSrc + 1];
        apos[vid + 2] = fullMeshData[0][vidSrc + 2];
        apos[vid + 3] = fullMeshData[0][vidSrc + 3];

        anorm[vid + 0] = fullMeshData[1][vidSrc + 0];
        anorm[vid + 1] = fullMeshData[1][vidSrc + 1];
        anorm[vid + 2] = fullMeshData[1][vidSrc + 2];
        anorm[vid + 3] = fullMeshData[1][vidSrc + 3];

        assert((ind * 2) < apos.size());
        assert((v.xref * 2) < fullMeshData[2].size());
        auv[ind * 2 + 0] = fullMeshData[2][v.xref * 2 + 0];
        auv[ind * 2 + 1] = fullMeshData[2][v.xref * 2 + 1];

        atan[vid + 0] = fullMeshData[3][vidSrc + 0];
        atan[vid + 1] = fullMeshData[3][vidSrc + 1];
        atan[vid + 2] = fullMeshData[3][vidSrc + 2];
        atan[vid + 3] = fullMeshData[3][vidSrc + 3];

        fullMeshData[4][ind * 2 + 0] = v.uv[0] / float(aw);
        fullMeshData[4][ind * 2 + 1] = v.uv[1] / float(ah);
      }
      fullMeshData[MESH_ATTRIBUTE_TYPE_POSITION] = apos;
      fullMeshData[MESH_ATTRIBUTE_TYPE_NORMAL] = anorm;
      fullMeshData[MESH_ATTRIBUTE_TYPE_UV] = auv;
      fullMeshData[MESH_ATTRIBUTE_TYPE_TANGENT] = atan;

      //setting the stride for the uvs
      strides[MESH_ATTRIBUTE_TYPE_UV_LIGHTMAP] = static_cast<float>(
              MESH_ATTRIBUTE_SIZE_IN_BYTES[MESH_ATTRIBUTE_TYPE_UV_LIGHTMAP] / 4u);

      auto t2 = std::chrono::high_resolution_clock::now();
      auto secs= std::chrono::duration_cast<std::chrono::seconds>(t2 - t1);
      printf("Generating atlas for mesh %s took %llds\n",outMesh.name.c_str(),secs.count());


    }
  }


  std::vector<uint32_t> inIndices = outMesh.indices;
  SirMetal::optimizeVertexCache(outMesh.indices, inIndices, outMesh.indices.size(),
                                fullMeshData[0].size());

  // merge the buffer into a single one
  int attributesCount = 4;
  //if we have the uv maps we have an extra attributes. this is good enough until we have skinning, then it will be trickier
  attributesCount += generateLightUVs ? 1 : 0;
  SirMetal::mergeRawMeshBuffers(fullMeshData, strides, attributesCount, outMesh.vertices,
                                outMesh.ranges);

  outMesh.name = mesh->name;

  return true;
}

}// namespace SirMetal