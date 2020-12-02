#include "SirMetal/resources/shaderManager.h"
#import "SirMetal/io/file.h"
#import <Metal/Metal.h>

namespace SirMetal {
LibraryHandle ShaderManager::loadShader(const char *path) {

  // checking if is already loaded
  const std::string fileName = getFileName(path);
  auto found = m_nameToLibraryHandle.find(fileName);
  if (found != m_nameToLibraryHandle.end()) {
    return LibraryHandle{found->second};
  }

  // loading the actual shader library
  NSString *shaderPath =
      [NSString stringWithCString:path
                         encoding:[NSString defaultCStringEncoding]];
  __autoreleasing NSError *errorLib = nil;

  NSString *content = [NSString stringWithContentsOfFile:shaderPath
                                                encoding:NSUTF8StringEncoding
                                                   error:nil];
  id<MTLDevice> currDevice = m_device;
  id<MTLLibrary> libraryRaw =
      [currDevice newLibraryWithSource:content options:nil error:&errorLib];
  //[currDevice newLibraryWithSource:content options:nil error:&errorLib];

  if (libraryRaw == nil) {
    NSString *errorStr = [errorLib localizedDescription];
    printf("[ERROR] Error in compiling shader %s:\n %s", fileName.c_str(),
                   [errorStr UTF8String]);
    return {};
  }

  ShaderMetadata metadata{};
  bool result = generateLibraryMetadata(libraryRaw, metadata, path);
  if (!result) {
    return {};
  }

  uint32_t index = m_libraryCounter++;

  // updating the look ups
  m_libraries[index] = metadata;
  m_nameToLibraryHandle[fileName] = index;

  return getHandle<LibraryHandle>(index);
}

id ShaderManager::getLibraryFromHandle(LibraryHandle handle) {
  uint32_t index = getIndexFromHandle(handle);
  return m_libraries[index].library;
}

LibraryHandle ShaderManager::getHandleFromName(const std::string &name) const {
  auto found = m_nameToLibraryHandle.find(name);
  if (found != m_nameToLibraryHandle.end()) {
    return getHandle<LibraryHandle>(found->second);
  }
  return {};
}

bool ShaderManager::generateLibraryMetadata(
    id library, ShaderManager::ShaderMetadata &metadata,
    const char *libraryPath) {

  id<MTLLibrary> lib = library;
  NSArray<NSString *> *names = [lib functionNames];
  int namesCount = (int)[names count];

  for (int i = 0; i < namesCount; ++i) {
    id<MTLFunction> fn = [lib newFunctionWithName:names[i]];
    MTLFunctionType type = fn.functionType;
    switch (type) {
    case (MTLFunctionType::MTLFunctionTypeVertex): {
      if (metadata.vertexFn != nullptr) {
        printf("[WARN] Shader library at path %s\ncontains multiple vertex "
                      "functions, first found %s,"
                      " then found %s, ignoring second one...",
                      libraryPath, [[metadata.vertexFn name] UTF8String],
                      [names[i] UTF8String]);

      } else {
        metadata.vertexFn = fn;
      }
      break;
    }
    case (MTLFunctionType::MTLFunctionTypeFragment): {
      if (metadata.fragFn != nullptr) {
        printf("[WARN] Shader library at path%s\ncontains multiple fragment "
                      "functions, first found %s,"
                      " then found %s, ignoring second one...",
                      libraryPath, [[metadata.fragFn name] UTF8String],
                      [names[i] UTF8String]);

      } else {
        metadata.fragFn = fn;
      }
      break;
    }
    case (MTLFunctionType::MTLFunctionTypeKernel): {
      if (metadata.computeFn != nullptr) {
        printf("[WARN] Shader library at path %s\ncontains multiple compute "
                      "functions, first found %s,"
                      " then found %s, ignoring second one...",
                      libraryPath, [[metadata.computeFn name] UTF8String],
                      [names[i] UTF8String]);

      } else {
        metadata.computeFn = fn;
      }
      break;
    }
    }
  }

  if (metadata.computeFn != nullptr) {
    // lib is compute
    metadata.type = SHADER_TYPE::COMPUTE;
  } else {
    if ( metadata.vertexFn != nullptr) {
      metadata.type = SHADER_TYPE::RASTER;
    } else {
      printf("[ERROR] The library at path %s\ndoes not provided a complete "
                     "combination of vertex and fragment shader",
                     libraryPath);
      return false;
    }
  }

  metadata.libraryPath = libraryPath;
  metadata.library = library;

  return true;
}

/*
MTLRenderPipelineReflection* reflectionObj;
MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo |
MTLPipelineOptionArgumentInfo; id <MTLRenderPipelineState> pso = [_device
newRenderPipelineStateWithDescriptor:pipelineDescriptor options:option
reflection:&reflectionObj error:&error];

for (MTLArgument *arg in reflectionObj.vertexArguments)
{
    NSLog(@"Found arg: %@\n", arg.name);

    if (arg.bufferDataType == MTLDataTypeStruct)
    {
        for( MTLStructMember* uniform in arg.bufferStructType.members )
        {
            NSLog(@"uniform: %@ type:%lu, location: %lu", uniform.name,
(unsigned long)uniform.dataType, (unsigned long)uniform.offset);
        }
    }
}
*/

id ShaderManager::getVertexFunction(LibraryHandle handle) {
  uint32_t index = getIndexFromHandle(handle);
  assert(m_libraries.find(index) != m_libraries.end());
  return m_libraries[index].vertexFn;
}
id ShaderManager::getFragmentFunction(LibraryHandle handle) {
  assert(handle.isHandleValid());
  assert(getTypeFromHandle(handle) == LibraryHandle::type);
  uint32_t index = getIndexFromHandle(handle);
  assert(m_libraries.find(index) != m_libraries.end());
  return m_libraries[index].fragFn;
}

} // namespace SirMetal
