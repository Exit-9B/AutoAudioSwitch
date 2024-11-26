cmake_minimum_required(VERSION 3.24)

project(
	AutoAudioSwitch
	VERSION 1.0.3
	LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_IGNORE_PATH "${PROJECT_BINARY_DIR}")
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/SKSE-CMakeModules/Modules"
)
include(SKSEPlugin)

SKSEPlugin_Add(
	${PROJECT_NAME}
	SOURCE_DIR src
	SOURCES
		.clang-format
		.editorconfig
		README.md
		vcpkg.json
	PRECOMPILE_HEADERS
		src/common/PCH.h
)

find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${SIMPLEINI_INCLUDE_DIRS}
)

set_target_properties(CommonLibSSE PROPERTIES
	FOLDER External
)

install(
	FILES
		"${PROJECT_SOURCE_DIR}/data/Interface/Translations/${PROJECT_NAME}_ENGLISH.txt"
	DESTINATION "Interface/Translations"
)

set(CPACK_GENERATOR "7Z")
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION})
if(BUILD_SKYRIMVR)
	string(APPEND CPACK_PACKAGE_FILE_NAME "-VR")
endif()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
include(CPack)
