#include "SirMetal/resources/gltfLoader.h"
#include "SirMetal/engine.h"
#include "SirMetal/io/fileUtils.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/textureManager.h"
#include <SirMetal/core/mathUtils.h>
#include <simd/simd.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

namespace SirMetal {

simd_float4x4 toSimdMatrix(const cgltf_float data[16]) {
  vector_float4 X = {data[0], data[1], data[2], data[3]};
  vector_float4 Y = {data[4], data[5], data[6], data[7]};
  vector_float4 Z = {data[8], data[9], data[10], data[11]};
  vector_float4 W = {data[12], data[13], data[14], data[15]};

  matrix_float4x4 mat = {X, Y, Z, W};
  return mat;
}

simd_float4x4 getMatrix(const cgltf_node &node) {
  if (node.has_matrix) {
    return toSimdMatrix(node.matrix);
  }

  simd_float3 t = node.has_translation
                      ? simd_float3{node.translation[0], node.translation[1],
                                    node.translation[2]}
                      : simd_float3{0, 0, 0};
  simd_quatf r = node.has_rotation
                     ? simd_quaternion(node.rotation[0], node.rotation[1],
                                       node.rotation[2], node.rotation[3])
                     : simd_quaternion(0.0f, simd_float3{0, 1, 0});

  simd_float3 s = node.has_scale
                      ? simd_float3{node.scale[0], node.scale[1], node.scale[2]}
                      : simd_float3{1, 1, 1};
  return getMatrixFromComponents(t, r, s);
}

GLTFMaterial loadMaterial(EngineContext *context,
                          const cgltf_material *material) {
  GLTFMaterial outMaterial;
  outMaterial.name = material->name;
  outMaterial.doubleSided = true;
  const auto &pbr = material->pbr_metallic_roughness;
  auto colorFactor = pbr.base_color_factor;
  outMaterial.colorFactors = simd_float4{colorFactor[0], colorFactor[1],
                                         colorFactor[2], colorFactor[3]};
  outMaterial.colorTexture = pbr.base_color_texture.texture
                                 ? context->m_textureManager->loadFromMemory(
                                       pbr.base_color_texture.texture,
                                       LOAD_TEXTURE_TYPE::GLTF_TEXTURE, true)
                                 : TextureHandle{};

  return outMaterial;
}

void loadNode(EngineContext *context, const cgltf_node *node,
              GLTFAsset &outAsset, GLTFLoadFlags flags,
              simd_float4x4 parentMatrix) {
  Model model{};
  if (node->mesh != nullptr) {
    printf("loading mesh for node %s\n", node->name);
    assert(node->mesh->primitives_count == 1 &&
           "gltf loader does not support multiple primitives per mesh yet");
    model.mesh = context->m_meshManager->loadFromMemory(
        node->mesh, LOAD_MESH_TYPE::GLTF_MESH);

    assert(node->mesh->primitives_count == 1);
    if (node->mesh->primitives[0].material != nullptr) {
      model.material =
          loadMaterial(context, node->mesh->primitives[0].material);
    }
  }

  simd_float4x4 matrix = getMatrix(*node);
  model.matrix = simd_mul(parentMatrix, matrix);

  bool flatten = (flags & GLTF_LOAD_FLAGS_FLATTEN_HIERARCHY) > 0;
  bool isEmpty = node->mesh == nullptr;
  if (!(flatten & isEmpty)) {
    outAsset.models.push_back(model);
  }

  for (int c = 0; c < node->children_count; ++c) {
    const auto *child = node->children[c];
    loadNode(context, child, outAsset, flags, model.matrix);
  }
}

bool loadGLTF(EngineContext *context, const char *path, GLTFAsset &outAsset,
              GLTFLoadFlags flags) {
  assert(((flags & GLTF_LOAD_FLAGS_FLATTEN_HIERARCHY) > 0) &&
         "only flatten hierarchy supported for now");
  assert(fileExists(path));
  cgltf_options options = {};
  cgltf_data *data = nullptr;
  cgltf_result result = cgltf_parse_file(&options, path, &data);
  if (result != cgltf_result_success) {
    printf("[Error] Error loading gltf file from %s\n", path);
    return false;
  }

  result = cgltf_load_buffers(&options, data, path);
  if (result != cgltf_result_success) {
    cgltf_free(data);
    printf("[Error] Error loading gltf buffers from %s\n", path);
    return false;
  }

  printf("Loading gltf file %s\n", path);

  cgltf_scene *scene = data->scene;
  // iterate the the scene
  int nodesCount = scene->nodes_count;
  for (int i = 0; i < nodesCount; ++i) {
    auto *node = scene->nodes[i];
    printf("Node -> %s\n", node->name);
    loadNode(context, node, outAsset, flags, getIdentity());
  }

  cgltf_free(data);
  return true;
}

} // namespace SirMetal