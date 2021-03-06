cmake_minimum_required(VERSION 3.3)

project(NesEmulator)

set(CMAKE_BUILD_TYPE Debug)

set(BUILD_ROOT "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

set(SourceDir ${PROJECT_SOURCE_DIR}/src)

file(GLOB SOURCES "src/*")

include_directories(include)

add_executable(NesEmulator ${SOURCES})

SET(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/Windows/x86")

TARGET_LINK_LIBRARIES(NesEmulator ${LIB_DIR}/SDL2_image.lib)
TARGET_LINK_LIBRARIES(NesEmulator ${LIB_DIR}/SDL2main.lib)
TARGET_LINK_LIBRARIES(NesEmulator ${LIB_DIR}/SDL2.lib)
TARGET_LINK_LIBRARIES(NesEmulator ${LIB_DIR}/SDL2_mixer.lib)

set (OUT_DIR ${IN_DIR})

# ----- DLL ------------------------------------------------------------------
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if (${CMAKE_GENERATOR} MATCHES "Visual Studio 14")
		 set (DLL_TO_DBG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
		 set (DLL_TO_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
	else()
		 set (DLL_TO_DBG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
		 set (DLL_TO_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	endif()

	file(MAKE_DIRECTORY ${DLL_TO_DBG})
	file(MAKE_DIRECTORY ${DLL_TO_RELEASE})
	#message(${DLL_TO_DBG})
	file(MAKE_DIRECTORY ${DLL_TO_DBG})
	file(GLOB_RECURSE nativeLibsDebug bin/Windows/x86/Debug/*.dll)
	file(GLOB_RECURSE nativeLibsRelease bin/Windows/x86/Debug/*.dll)
	foreach(AddonFile ${nativeLibsDebug})
		message(${AddonFile})
		get_filename_component(OUT_DIR ${AddonFile} DIRECTORY)
		string(REGEX REPLACE "${CMAKE_CURRENT_LIST_DIR}/../bin/Windows/x86/Debug"
				"" OUT_DIR ${OUT_DIR})
		add_custom_command(TARGET GET NesEmulator PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E
				copy ${AddonFile} ${DLL_TO_DBG}/)
	endforeach(AddonFile)
	foreach(AddonFile ${nativeLibsRelease})
		get_filename_component(OUT_DIR ${AddonFile} DIRECTORY)
		string(REGEX REPLACE "${CMAKE_CURRENT_LIST_DIR}/../bin/Windows/x86/Release"
				"" OUT_DIR ${OUT_DIR})
		add_custom_command(TARGET GET NesEmulator PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E
				copy ${AddonFile} ${DLL_TO_RELEASE}/)
	endforeach(AddonFile)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
