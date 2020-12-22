cmake_minimum_required(VERSION 3.13.0)

project(tests)

    #looking for  files
	file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")


    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(
						${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_SOURCE_DIR}/engine/src
						${CMAKE_SOURCE_DIR}/vendors
						${CMAKE_SOURCE_DIR}/vendors/farmhash
	)

	file(COPY "testData" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )

	# Project Libs
	set(LINK_LIBS)
	set(MAC_LIBS Metal MetalKit Foundation Cocoa MetalPerformanceShaders)
	foreach (LIB ${MAC_LIBS})
		find_library(${LIB}_LIBRARY ${LIB})
		list(APPEND LINK_LIBS ${${LIB}_LIBRARY})
		mark_as_advanced(${${LIB}_LIBRARY})
	endforeach ()

	#adding the executable
    add_executable(${PROJECT_NAME} ${SOURCE_FILES})
	target_link_libraries(${PROJECT_NAME} ${LINK_LIBS} ${SDL2_LIBRARIES} SirMetalLib)

	set_property (TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY
			COMPILE_FLAGS "-fobjc-arc")
