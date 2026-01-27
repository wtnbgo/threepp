# Generic Canvas Backend for threepp

The Generic backend allows you to use threepp with any window system or graphics framework by implementing a set of interface functions. This is useful when integrating threepp into existing applications or using window systems not directly supported (like Qt, wxWidgets, Win32, or custom engines).

## Building with Generic Backend

To build threepp with the Generic backend, configure CMake with:

```bash
cmake -DTHREEPP_WITH_GENERIC=ON ..
```

**Note:** The Generic backend is mutually exclusive with GLFW and SDL3 backends. Only one can be enabled at a time.

## Architecture Overview

The Generic backend consists of two main interfaces:

1. **WindowSystemInterface** - Core window operations (required)
2. **ImGuiBackendInterface** - ImGui integration (optional)

Both interfaces use `std::function` callbacks that you implement and register before creating any Canvas objects.

## Event System

All input and window events are represented through the unified `Event` structure:

```cpp
namespace threepp::generic {

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

struct Event {
    EventType type;
    std::variant<...> data;  // Event-specific data
    const void* rawEvent;    // Optional: platform-specific event pointer
};

}
```

### Event Data Structures

| Event Type | Data Structure | Fields |
|------------|----------------|--------|
| `WindowResize` | `WindowResizeEventData` | `width`, `height` |
| `WindowMove` | `WindowMoveEventData` | `x`, `y` |
| `KeyDown/KeyUp` | `KeyEventData` | `key`, `scancode`, `mods`, `repeat` |
| `MouseButtonDown/Up` | `MouseButtonEventData` | `button` (0=left, 1=middle, 2=right), `x`, `y` |
| `MouseMove` | `MouseMoveEventData` | `x`, `y` |
| `MouseWheel` | `MouseWheelEventData` | `xOffset`, `yOffset` |
| `FileDrop` | `FileDropEventData` | `paths` (vector of strings) |

### Creating Events

Use the static factory methods for convenience:

```cpp
using namespace threepp::generic;

Event::windowClose();
Event::windowResize(800, 600);
Event::keyDown(Key::A, scancode, mods, false);
Event::mouseButtonDown(0, x, y);  // Left button
Event::mouseMove(x, y);
Event::mouseWheel(0.0f, 1.0f);
Event::fileDrop({"path/to/file.obj"});
```

## WindowSystemInterface

This is the main interface you must implement:

```cpp
#include <threepp/canvas/Generic.hpp>

threepp::generic::WindowSystemInterface wsi;

// Required: System initialization
wsi.initialize = []() -> bool {
    // Initialize your window system
    return true;
};

// Required: System shutdown
wsi.shutdown = []() {
    // Cleanup your window system
};

// Required: Create a window and OpenGL context
wsi.createWindow = [](const threepp::generic::WindowCreateParams& params) 
    -> threepp::generic::WindowHandle {
    // params.title, params.width, params.height
    // params.antialiasing, params.vsync, params.resizable, params.visible
    // Return your window handle (void*)
    return myWindowHandle;
};

// Required: Destroy window
wsi.destroyWindow = [](threepp::generic::WindowHandle window) {
    // Destroy the window
};

// Required: Get window size
wsi.getWindowSize = [](threepp::generic::WindowHandle window) 
    -> threepp::WindowSize {
    return {width, height};
};

// Optional: Set window size
wsi.setWindowSize = [](threepp::generic::WindowHandle window, int w, int h) {
    // Resize window
};

// Optional: Set window title
wsi.setWindowTitle = [](threepp::generic::WindowHandle window, 
                        const std::string& title) {
    // Set title
};

// Optional: Set window icon (RGBA pixel data)
wsi.setWindowIcon = [](threepp::generic::WindowHandle window,
                       const unsigned char* pixels, int w, int h) {
    // Set icon from RGBA data
};

// Optional: Show/hide window
wsi.setWindowVisible = [](threepp::generic::WindowHandle window, bool visible) {
    // Show or hide
};

// Required: Make OpenGL context current
wsi.makeContextCurrent = [](threepp::generic::WindowHandle window) {
    // Make GL context current for this window
};

// Required: Swap buffers
wsi.swapBuffers = [](threepp::generic::WindowHandle window) {
    // Swap front/back buffers
};

// Required: Poll for events
wsi.pollEvent = [](threepp::generic::WindowHandle window, 
                   threepp::generic::Event& event) -> bool {
    // Fill event structure if event available
    // Return true if event was filled, false if no more events
    if (hasEvent) {
        event = threepp::generic::Event::keyDown(...);
        return true;
    }
    return false;
};

// Optional: Set VSync
wsi.setVSync = [](threepp::generic::WindowHandle window, bool enabled) {
    // Enable/disable vsync
};

// Required: Get monitor count
wsi.getMonitorCount = []() -> int {
    return 1;
};

// Required: Get monitor size
wsi.getMonitorSize = [](int monitorIndex) -> threepp::WindowSize {
    return {1920, 1080};
};

// Optional: Get monitor DPI scale
wsi.getMonitorContentScale = [](int monitorIndex) -> std::pair<float, float> {
    return {1.0f, 1.0f};
};

// Optional: Get which monitor the window is on
wsi.getWindowMonitor = [](threepp::generic::WindowHandle window) -> int {
    return 0;
};

// Optional: Load OpenGL functions (if not using glad)
wsi.loadGLFunctions = []() {
    // Load GL functions if needed
    // threepp already includes glad, so this may be empty
};

// Register the interface
threepp::generic::setWindowSystemInterface(wsi);
```

## ImGuiBackendInterface (Optional)

If you want to use ImGui with the Generic backend, implement this interface:

```cpp
threepp::generic::ImGuiBackendInterface imgui;

imgui.initForOpenGL = [](threepp::generic::WindowHandle window) -> bool {
    // Initialize ImGui for your platform
    // This replaces ImGui_ImplXXX_InitForOpenGL()
    return true;
};

imgui.processEvent = [](const threepp::generic::Event& event) {
    // Convert generic event to ImGui input
    // This replaces ImGui_ImplXXX_ProcessEvent()
    ImGuiIO& io = ImGui::GetIO();
    
    switch (event.type) {
        case threepp::generic::EventType::KeyDown: {
            auto& data = std::get<threepp::generic::KeyEventData>(event.data);
            // io.AddKeyEvent(...);
            break;
        }
        case threepp::generic::EventType::MouseMove: {
            auto& data = std::get<threepp::generic::MouseMoveEventData>(event.data);
            io.AddMousePosEvent(data.x, data.y);
            break;
        }
        // ... handle other events
    }
};

imgui.newFrame = []() {
    // Platform-specific new frame setup
    // This replaces ImGui_ImplXXX_NewFrame()
};

imgui.shutdown = []() {
    // Cleanup ImGui platform backend
    // This replaces ImGui_ImplXXX_Shutdown()
};

threepp::generic::setImGuiBackendInterface(imgui);
```

When building examples or your application with ImGui support, define:

```cmake
target_compile_definitions(your_target PRIVATE THREEPP_IMGUI_GENERIC)
```

## Complete Example

```cpp
#include <threepp/threepp.hpp>
#include <threepp/canvas/Generic.hpp>

// Your platform-specific headers
#include "my_window_system.h"

int main() {
    // 1. Implement and register the window system interface
    threepp::generic::WindowSystemInterface wsi;
    
    wsi.initialize = []() {
        return MyWindowSystem::init();
    };
    
    wsi.shutdown = []() {
        MyWindowSystem::shutdown();
    };
    
    wsi.createWindow = [](const auto& params) {
        return MyWindowSystem::createWindow(
            params.title, params.width, params.height,
            params.antialiasing, params.vsync, params.resizable
        );
    };
    
    wsi.destroyWindow = [](auto window) {
        MyWindowSystem::destroyWindow(window);
    };
    
    wsi.getWindowSize = [](auto window) {
        auto [w, h] = MyWindowSystem::getSize(window);
        return threepp::WindowSize{w, h};
    };
    
    wsi.makeContextCurrent = [](auto window) {
        MyWindowSystem::makeCurrent(window);
    };
    
    wsi.swapBuffers = [](auto window) {
        MyWindowSystem::swapBuffers(window);
    };
    
    wsi.pollEvent = [](auto window, threepp::generic::Event& event) {
        MyEvent myEvent;
        if (!MyWindowSystem::pollEvent(window, myEvent)) {
            return false;
        }
        
        // Convert to generic event
        switch (myEvent.type) {
            case MY_QUIT:
                event = threepp::generic::Event::windowClose();
                break;
            case MY_RESIZE:
                event = threepp::generic::Event::windowResize(
                    myEvent.width, myEvent.height);
                break;
            case MY_KEYDOWN:
                event = threepp::generic::Event::keyDown(
                    convertKey(myEvent.key), 
                    myEvent.scancode,
                    myEvent.mods,
                    myEvent.repeat);
                break;
            // ... etc
        }
        return true;
    };
    
    wsi.getMonitorCount = []() { return 1; };
    wsi.getMonitorSize = [](int) { 
        return threepp::WindowSize{1920, 1080}; 
    };
    
    // Register
    threepp::generic::setWindowSystemInterface(wsi);
    
    // 2. Now use threepp normally
    threepp::Canvas canvas("My App");
    
    auto scene = threepp::Scene::create();
    auto camera = threepp::PerspectiveCamera::create(75, canvas.aspect());
    camera->position.z = 5;
    
    auto geometry = threepp::BoxGeometry::create();
    auto material = threepp::MeshBasicMaterial::create();
    material->color = threepp::Color::green;
    auto mesh = threepp::Mesh::create(geometry, material);
    scene->add(mesh);
    
    threepp::GLRenderer renderer(canvas.size());
    
    canvas.onWindowResize([&](threepp::WindowSize size) {
        camera->aspect = size.aspect();
        camera->updateProjectionMatrix();
        renderer.setSize(size);
    });
    
    canvas.animate([&]() {
        mesh->rotation.y += 0.01f;
        renderer.render(*scene, *camera);
    });
    
    return 0;
}
```

## Key Mapping

When converting keyboard events, map your platform's key codes to `threepp::Key`:

```cpp
threepp::Key convertKey(int platformKey) {
    switch (platformKey) {
        case PLATFORM_KEY_A: return threepp::Key::A;
        case PLATFORM_KEY_B: return threepp::Key::B;
        // ... etc
        case PLATFORM_KEY_SPACE: return threepp::Key::SPACE;
        case PLATFORM_KEY_ESCAPE: return threepp::Key::ESCAPE;
        case PLATFORM_KEY_UP: return threepp::Key::UP;
        case PLATFORM_KEY_DOWN: return threepp::Key::DOWN;
        case PLATFORM_KEY_LEFT: return threepp::Key::LEFT;
        case PLATFORM_KEY_RIGHT: return threepp::Key::RIGHT;
        default: return threepp::Key::UNKNOWN;
    }
}
```

## Modifier Keys

Modifier keys are represented as a bitmask:

| Modifier | Bit Value |
|----------|-----------|
| Shift    | 1         |
| Ctrl     | 2         |
| Alt      | 4         |
| GUI/Super| 8         |

## Tips

1. **OpenGL Context**: Ensure your window system creates a compatible OpenGL context (3.3+ core profile recommended).

2. **Event Loop**: The `pollEvent` function should return events one at a time. Return `false` when no more events are available.

3. **Thread Safety**: All interface functions are called from the main thread.

4. **Raw Events**: If you need to pass platform-specific events (e.g., for ImGui), set the `rawEvent` pointer in your Event structure.

5. **Error Handling**: Return appropriate error values from interface functions. `createWindow` should return `nullptr` on failure.

## API Reference

### Checking Interface Availability

```cpp
// Check if interfaces are registered
if (threepp::generic::hasWindowSystemInterface()) {
    auto& wsi = threepp::generic::getWindowSystemInterface();
}

if (threepp::generic::hasImGuiBackendInterface()) {
    auto& imgui = threepp::generic::getImGuiBackendInterface();
}
```

### WindowCreateParams

```cpp
struct WindowCreateParams {
    std::string title = "threepp";
    int width = 800;
    int height = 600;
    int antialiasing = 2;
    bool vsync = true;
    bool resizable = true;
    bool visible = true;  // false for headless mode
};
```
