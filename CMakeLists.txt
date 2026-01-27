cmake_minimum_required(VERSION 3.19)
# UTF-8 source
add_compile_options("$<$<AND:$<C_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:C,CXX>>:/utf-8>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/Zc:__cplusplus>")

project(threepp)

include(CMakeDependentOption)

option(THREEPP_BUILD_EXAMPLES "Build examples" ON)
option(THREEPP_BUILD_TESTS "Build test suite" ON)
option(THREEPP_WITH_AUDIO "Build with Audio" ON)

option(THREEPP_WITH_FBX "Build with FBXLoader (OpenFBX)" OFF)
option(THREEPP_WITH_USD "Build with USDLoader (tinyusdz)" OFF)
option(THREEPP_WITH_RLTOOLS "Fetch RLtools (header-only deep RL) for the reinforcement-learning example" OFF)

option(THREEPP_TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(THREEPP_WITH_WGPU "Build with WebGPU backend (via wgpu-native)" OFF)
option(THREEPP_WITH_VULKAN "Build with Vulkan backend (requires Vulkan SDK)" OFF)
cmake_dependent_option(THREEPP_WGPU_GLSL_COMPAT
    "Enable GLSL->SPIR-V translation for ShaderMaterial on the Wgpu backend (requires glslang)"
    ON "THREEPP_WITH_WGPU" OFF)

cmake_dependent_option(THREEPP_WITH_INFERENCE "Build with Inference support" OFF "THREEPP_WITH_WGPU OR THREEPP_WITH_VULKAN" OFF)

# Window backend options - choose one: GLFW, SDL3 or Generic
option(THREEPP_WITH_SDL3 "Build with SDL3 frontend" OFF)
option(THREEPP_WITH_GENERIC "Build with Generic frontend (user-provided interface)" OFF)

# Force THREEPP_WITH_GLFW ON when targeting Emscripten (if SDL3/Generic not selected)
cmake_dependent_option(THREEPP_WITH_GLFW "Build with GLFW frontend" ON "NOT DEFINED EMSCRIPTEN;NOT THREEPP_WITH_SDL3;NOT THREEPP_WITH_GENERIC" OFF)
# Force THREEPP_USE_EXTERNAL_GLFW OFF if NOT THREEPP_WITH_GLFW
cmake_dependent_option(THREEPP_USE_EXTERNAL_GLFW "Use externally supplied GLFW" OFF "THREEPP_WITH_GLFW;NOT THREEPP_WITH_SDL3;NOT THREEPP_WITH_GENERIC" OFF)

# Validation: cannot enable multiple window backends
set(_WINDOW_BACKEND_COUNT 0)
if (THREEPP_WITH_GLFW)
    math(EXPR _WINDOW_BACKEND_COUNT "${_WINDOW_BACKEND_COUNT} + 1")
endif()
if (THREEPP_WITH_SDL3)
    math(EXPR _WINDOW_BACKEND_COUNT "${_WINDOW_BACKEND_COUNT} + 1")
endif()
if (THREEPP_WITH_GENERIC)
    math(EXPR _WINDOW_BACKEND_COUNT "${_WINDOW_BACKEND_COUNT} + 1")
endif()
if (_WINDOW_BACKEND_COUNT GREATER 1)
    message(FATAL_ERROR "Cannot enable multiple window backends. Please choose only one of: THREEPP_WITH_GLFW, THREEPP_WITH_SDL3, THREEPP_WITH_GENERIC.")
endif()
unset(_WINDOW_BACKEND_COUNT)

cmake_dependent_option(THREEPP_BUILD_EXAMPLE_PROJECTS "Build example projects" ON "THREEPP_BUILD_EXAMPLES" OFF)
cmake_dependent_option(THREEPP_FETCH_ASSIMP "Make assimp available for examples" OFF "THREEPP_BUILD_EXAMPLES" OFF)


# ==============================================================================
# Global internal configuration
# ==============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


# ==============================================================================
# Dependencies
# ==============================================================================


if (DEFINED EMSCRIPTEN)
    if ("${CMAKE_SYSTEM}" MATCHES "Linux" OR "${CMAKE_SYSTEM}" MATCHES "Darwin-*")
        find_package(OpenGL REQUIRED)
    endif ()
else ()

    if (THREEPP_WITH_SDL3)
        find_package(SDL3 CONFIG REQUIRED)
    elseif (THREEPP_USE_EXTERNAL_GLFW)
        find_package(glfw3 CONFIG REQUIRED)
    endif ()

endif ()


# ==============================================================================
# Targets
# ==============================================================================

include(cmake/FetchOptionalDeps.cmake)

add_subdirectory(src)

# Example/test assets live in a separate
# repo, fetched only when building examples or tests so library consumers stay
# lean. Point at a local checkout with -DFETCHCONTENT_SOURCE_DIR_THREEPP_DATA=<dir>.
if (THREEPP_BUILD_EXAMPLES OR THREEPP_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            threepp_data
            GIT_REPOSITORY https://github.com/markaren/threepp_data.git
            GIT_TAG main  # pin a release tag for reproducible builds
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(threepp_data)
    set(THREEPP_DATA_DIR "${threepp_data_SOURCE_DIR}" CACHE INTERNAL "threepp-data asset path")
endif ()

if (THREEPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

if (NOT DEFINED EMSCRIPTEN AND THREEPP_BUILD_TESTS)

    include(FetchContent)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)

    enable_testing()
    add_subdirectory(tests)
endif ()


# ==============================================================================
# Exports and remaining installation
# ==============================================================================

configure_package_config_file(cmake/config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/threepp-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/threepp"
        PATH_VARS THREEPP_USE_EXTERNAL_GLFW
        NO_SET_AND_CHECK_MACRO)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/threepp-config-version.cmake"
        VERSION 0.0.0
        COMPATIBILITY AnyNewerVersion)
install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/threepp-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/threepp-config-version.cmake"
        DESTINATION
        "${CMAKE_INSTALL_DATADIR}/threepp")

if (BUILD_SHARED_LIBS AND NOT THREEPP_USE_EXTERNAL_GLFW)
    install(TARGETS glfw EXPORT threepp-targets)
endif ()
if (THREEPP_WGPU_GLSL_COMPAT AND NOT EMSCRIPTEN)
    install(TARGETS glslang SPIRV glslang-default-resource-limits EXPORT threepp-targets)
endif ()
if (THREEPP_WITH_USD)
    install(TARGETS tinyusdz_static EXPORT threepp-targets)
    install(DIRECTORY "${tinyusdz_SOURCE_DIR}/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tinyusdz"
            FILES_MATCHING PATTERN "*.hh" PATTERN "*.h" PATTERN "*.inc")
endif ()
install(TARGETS threepp EXPORT threepp-targets)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(EXPORT threepp-targets
        NAMESPACE threepp::
        DESTINATION "${CMAKE_INSTALL_DATADIR}/threepp")
