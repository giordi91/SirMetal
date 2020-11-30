
cmake_minimum_required(VERSION 3.13.0)

project(sandbox)

find_package(SDL2 REQUIRED)

include_directories(MyProject
        ${SDL2_INCLUDE_DIRS}
        "${CMAKE_SOURCE_DIR}/engine/src"
        "${CMAKE_SOURCE_DIR}/vendors/json/include"
        )

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")
set_source_files_properties(${SOURCE_FILES} PROPERTIES
        COMPILE_FLAGS "-x objective-c++")

# Project Libs
set(LINK_LIBS)
set(MAC_LIBS Metal MetalKit Foundation Cocoa)
foreach (LIB ${MAC_LIBS})
    find_library(${LIB}_LIBRARY ${LIB})
    list(APPEND LINK_LIBS ${${LIB}_LIBRARY})
    mark_as_advanced(${${LIB}_LIBRARY})
endforeach ()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

#set_target_properties(${PROJECT_NAME} PROPERTIES
#        MACOSX_BUNDLE YES
#        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Resources/Info.plist
#        )
target_link_libraries(${PROJECT_NAME} ${LINK_LIBS} ${SDL2_LIBRARIES} SirMetalLib)


set_property (TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY
        COMPILE_FLAGS "-fobjc-arc")

