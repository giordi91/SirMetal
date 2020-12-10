cmake_minimum_required(VERSION 3.13.0)

project(SirMetalLib)

find_package(SDL2 REQUIRED)
#find_package(OpenMP)
#looking for  files
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")
set_source_files_properties(${SOURCE_FILES} PROPERTIES
        COMPILE_FLAGS "-x objective-c++")

include_directories(${PROJECT_NAME}
        ${SDL2_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/vendors/json/include"
        "${CMAKE_SOURCE_DIR}/vendors/meshoptimizer"
        )


file(GLOB_RECURSE SHADER_FILES "../data/sandbox/*.metal")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -fopenmp")

# Project Libs
set(LINK_LIBS)
set(MAC_LIBS Metal MetalKit Foundation Cocoa MetalPerformanceShaders)
foreach (LIB ${MAC_LIBS})
    find_library(${LIB}_LIBRARY ${LIB})
    list(APPEND LINK_LIBS ${${LIB}_LIBRARY})
    mark_as_advanced(${${LIB}_LIBRARY})
endforeach ()

#adding the executable
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES} ${INCLUDES_FILES} ${SHADER_FILES})
target_link_libraries(${PROJECT_NAME} ${LINK_LIBS} ${SDL2_LIBRARIES} meshoptimizer )
#OpenMP::OpenMP_CXX



set_property (TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY
        COMPILE_FLAGS "-fobjc-arc")

