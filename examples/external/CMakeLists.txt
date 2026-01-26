
add_library(imgui)
add_library(imgui::imgui ALIAS imgui)

# Common imgui sources
set(IMGUI_COMMON_SOURCES
        "imgui/imgui.cpp"
        "imgui/imgui_draw.cpp"
        "imgui/imgui_impl_opengl3.cpp"
        "imgui/imgui_tables.cpp"
        "imgui/imgui_widgets.cpp"
)

if (THREEPP_WITH_SDL3)
    # SDL3 backend
    target_sources(imgui PRIVATE
            ${IMGUI_COMMON_SOURCES}
            "imgui/imgui_impl_sdl3.cpp"
    )
    target_link_libraries(imgui PRIVATE SDL3::SDL3)
    target_compile_definitions(imgui PUBLIC THREEPP_IMGUI_SDL3=1)
else()
    # GLFW backend (default)
    target_sources(imgui PRIVATE
            ${IMGUI_COMMON_SOURCES}
            "imgui/imgui_impl_glfw.cpp"
    )
    target_link_libraries(imgui PRIVATE glfw)
    target_compile_definitions(imgui PUBLIC THREEPP_IMGUI_GLFW=1)
endif()

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

if (THREEPP_WITH_WGPU)
    target_sources(imgui PRIVATE "imgui/imgui_impl_wgpu.cpp")
    target_compile_definitions(imgui PUBLIC THREEPP_WITH_WGPU=1)
    if (EMSCRIPTEN)
        target_compile_options(imgui PUBLIC "--use-port=emdawnwebgpu")
        target_link_options(imgui PUBLIC "--use-port=emdawnwebgpu")
        target_compile_definitions(imgui PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_DAWN=1)
    else()
        target_include_directories(imgui PUBLIC "${WGPU_INCLUDE_DIR}")
        target_compile_definitions(imgui PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_WGPU=1)
    endif()
endif()

if (THREEPP_WITH_VULKAN)
    find_package(Vulkan REQUIRED)
    target_sources(imgui PRIVATE "imgui/imgui_impl_vulkan.cpp")
    target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
    target_compile_definitions(imgui PUBLIC THREEPP_WITH_VULKAN=1)
endif()
