#ifndef THREEPP_CANVAS_GENERIC_HPP
#define THREEPP_CANVAS_GENERIC_HPP

#include "threepp/canvas/WindowSize.hpp"
#include "threepp/input/KeyListener.hpp"
#include "threepp/math/Vector2.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace threepp::generic {

    //=====================================================================
    // Event Definitions
    //=====================================================================

    /** Event type */
    enum class EventType {
        None,
        WindowClose,
        WindowResize,
        WindowMove,
        KeyDown,
        KeyUp,
        MouseButtonDown,
        MouseButtonUp,
        MouseMove,
        MouseWheel,
        FileDrop
    };

    /** Window resize event data */
    struct WindowResizeEventData {
        int width;
        int height;
    };

    /** Window move event data */
    struct WindowMoveEventData {
        int x;
        int y;
    };

    /** Key event data */
    struct KeyEventData {
        Key key;
        int scancode;
        int mods;
        bool repeat;
    };

    /** Mouse button event data */
    struct MouseButtonEventData {
        int button;  // 0: left, 1: middle, 2: right
        float x;
        float y;
    };

    /** Mouse move event data */
    struct MouseMoveEventData {
        float x;
        float y;
    };

    /** Mouse wheel event data */
    struct MouseWheelEventData {
        float xOffset;
        float yOffset;
    };

    /** File drop event data */
    struct FileDropEventData {
        std::vector<std::string> paths;
    };

    /** Unified event structure */
    struct Event {
        EventType type = EventType::None;
        std::variant<
            std::monostate,
            WindowResizeEventData,
            WindowMoveEventData,
            KeyEventData,
            MouseButtonEventData,
            MouseMoveEventData,
            MouseWheelEventData,
            FileDropEventData
        > data;

        // Pointer to raw platform-specific event (if needed)
        const void* rawEvent = nullptr;

        static Event windowClose() {
            return {EventType::WindowClose, std::monostate{}, nullptr};
        }

        static Event windowResize(int w, int h) {
            return {EventType::WindowResize, WindowResizeEventData{w, h}, nullptr};
        }

        static Event windowMove(int x, int y) {
            return {EventType::WindowMove, WindowMoveEventData{x, y}, nullptr};
        }

        static Event keyDown(Key key, int scancode, int mods, bool repeat) {
            return {EventType::KeyDown, KeyEventData{key, scancode, mods, repeat}, nullptr};
        }

        static Event keyUp(Key key, int scancode, int mods) {
            return {EventType::KeyUp, KeyEventData{key, scancode, mods, false}, nullptr};
        }

        static Event mouseButtonDown(int button, float x, float y) {
            return {EventType::MouseButtonDown, MouseButtonEventData{button, x, y}, nullptr};
        }

        static Event mouseButtonUp(int button, float x, float y) {
            return {EventType::MouseButtonUp, MouseButtonEventData{button, x, y}, nullptr};
        }

        static Event mouseMove(float x, float y) {
            return {EventType::MouseMove, MouseMoveEventData{x, y}, nullptr};
        }

        static Event mouseWheel(float xOffset, float yOffset) {
            return {EventType::MouseWheel, MouseWheelEventData{xOffset, yOffset}, nullptr};
        }

        static Event fileDrop(std::vector<std::string> paths) {
            return {EventType::FileDrop, FileDropEventData{std::move(paths)}, nullptr};
        }
    };

    //=====================================================================
    // Window Creation Parameters
    //=====================================================================

    struct WindowCreateParams {
        std::string title = "threepp";
        int width = 800;
        int height = 600;
        int antialiasing = 2;
        bool vsync = true;
        bool resizable = true;
        bool visible = true;  // headless = !visible
    };

    //=====================================================================
    // Window Handle (opaque type)
    //=====================================================================
    using WindowHandle = void*;

    //=====================================================================
    // Generic Window System Interface
    //=====================================================================

    struct WindowSystemInterface {
        /** Initialize the system */
        std::function<bool()> initialize;

        /** Shutdown the system */
        std::function<void()> shutdown;

        /** Create a window */
        std::function<WindowHandle(const WindowCreateParams&)> createWindow;

        /** Destroy a window */
        std::function<void(WindowHandle)> destroyWindow;

        /** Get window size */
        std::function<WindowSize(WindowHandle)> getWindowSize;

        /** Set window size */
        std::function<void(WindowHandle, int width, int height)> setWindowSize;

        /** Set window title */
        std::function<void(WindowHandle, const std::string&)> setWindowTitle;

        /** Set window icon (RGBA pixel data) */
        std::function<void(WindowHandle, const unsigned char* pixels, int width, int height)> setWindowIcon;

        /** Set window visibility */
        std::function<void(WindowHandle, bool visible)> setWindowVisible;

        /** Set vertical sync */
        std::function<void(WindowHandle, bool enabled)> setVSync;

        /** Make OpenGL context current */
        std::function<void(WindowHandle)> makeContextCurrent;

        /** Swap buffers */
        std::function<void(WindowHandle)> swapBuffers;

        /** Poll events */
        std::function<bool(WindowHandle, Event&)> pollEvent;

        /** Get monitor count */
        std::function<int()> getMonitorCount;

        /** Get monitor size */
        std::function<WindowSize(int monitorIndex)> getMonitorSize;

        /** Get monitor content scale */
        std::function<std::pair<float, float>(int monitorIndex)> getMonitorContentScale;

        /** Get the monitor index where the window is located */
        std::function<int(WindowHandle)> getWindowMonitor;

        /** Load OpenGL functions (e.g., glad) */
        std::function<void()> loadGLFunctions;
    };

    //=====================================================================
    // ImGui Backend Interface
    //=====================================================================

    struct ImGuiBackendInterface {
        /** Initialize ImGui backend for OpenGL */
        std::function<bool(WindowHandle)> initForOpenGL;

        /** Process event */
        std::function<void(const Event&)> processEvent;

        /** Begin new frame */
        std::function<void()> newFrame;

        /** Shutdown */
        std::function<void()> shutdown;
    };

    //=====================================================================
    // Global Interface Registration and Retrieval
    //=====================================================================

    /** Register window system interface */
    void setWindowSystemInterface(const WindowSystemInterface& iface);

    /** Get window system interface */
    WindowSystemInterface& getWindowSystemInterface();

    /** Check if window system interface is registered */
    bool hasWindowSystemInterface();

    /** Register ImGui backend interface */
    void setImGuiBackendInterface(const ImGuiBackendInterface& iface);

    /** Get ImGui backend interface */
    ImGuiBackendInterface& getImGuiBackendInterface();

    /** Check if ImGui backend interface is registered */
    bool hasImGuiBackendInterface();

}// namespace threepp::generic

#endif//THREEPP_CANVAS_GENERIC_HPP
