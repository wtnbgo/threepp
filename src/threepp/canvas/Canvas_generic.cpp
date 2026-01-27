#include "threepp/canvas/Canvas.hpp"
#include "threepp/canvas/Generic.hpp"
#include "threepp/canvas/Monitor.hpp"

#include "threepp/favicon.hpp"
#include "threepp/loaders/ImageLoader.hpp"
#include "threepp/utils/StringUtils.hpp"

#include <glad/glad.h>

#include <iostream>
#include <optional>
#include <queue>

using namespace threepp;
using namespace threepp::generic;

namespace {

    void setWindowIcon(WindowHandle window, std::optional<std::filesystem::path> customIcon) {
        auto& wsi = getWindowSystemInterface();
        if (!wsi.setWindowIcon) return;

        ImageLoader imageLoader;
        std::optional<Image> favicon;
        if (customIcon) {
            favicon = imageLoader.load(*customIcon, 4, false);
        } else {
            favicon = imageLoader.load(faviconSource(), 4, false);
        }
        if (favicon) {
            wsi.setWindowIcon(window, favicon->data().data(),
                static_cast<int>(favicon->width),
                static_cast<int>(favicon->height));
        }
    }

}// namespace

struct Canvas::Impl {

    Canvas& scope;
    WindowHandle window = nullptr;

    WindowSize size_;
    Vector2 lastMousePos_;

    bool close_{false};
    bool exitOnKeyEscape_;

    std::vector<std::function<void(WindowSize)>> resizeListener;
    std::vector<std::function<void(int monitor)>> monitorChangesListener;
    std::vector<std::function<void(const void *event)>> rawEventListeners;

    explicit Impl(Canvas& scope, const Parameters& params)
        : scope(scope), exitOnKeyEscape_(params.exitOnKeyEscape_) {

        auto& wsi = getWindowSystemInterface();

        if (!wsi.initialize || !wsi.initialize()) {
            std::cerr << "Error: Window system initialization failed" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (params.size_) {
            size_ = *params.size_;
        } else {
            const auto fullSize = monitor::monitorSize();
            size_ = {fullSize.width() / 2, fullSize.height() / 2};
        }

        WindowCreateParams createParams;
        createParams.title = params.title_;
        createParams.width = size_.width();
        createParams.height = size_.height();
        createParams.antialiasing = params.antialiasing_;
        createParams.vsync = params.vsync_;
        createParams.resizable = params.resizable_;
        createParams.visible = !params.headless_;

        window = wsi.createWindow(createParams);
        if (!window) {
            std::cerr << "Error: Window creation failed" << std::endl;
            exit(EXIT_FAILURE);
        }

        wsi.makeContextCurrent(window);

        setWindowIcon(window, params.favicon_);

        if (wsi.loadGLFunctions) {
            wsi.loadGLFunctions();
        }

        if (wsi.setVSync) {
            wsi.setVSync(window, params.vsync_);
        }

        if (params.antialiasing_ > 0) {
            glEnable(GL_MULTISAMPLE);
        }

        glEnable(GL_PROGRAM_POINT_SIZE);
    }

    [[nodiscard]] const WindowSize& getSize() const {
        return size_;
    }

    void setSize(std::pair<int, int> size) const {
        auto& wsi = getWindowSystemInterface();
        if (wsi.setWindowSize) {
            wsi.setWindowSize(window, size.first, size.second);
        }
    }

    void processEvents() {
        auto& wsi = getWindowSystemInterface();
        if (!wsi.pollEvent) return;

        Event event;
        while (wsi.pollEvent(window, event)) {
            handleEvent(event);
        }
    }

    void handleEvent(const Event& event) {
        // Forward raw event to listeners (e.g., ImGUI)
        if (event.rawEvent) {
            for (const auto& listener : rawEventListeners) {
                listener(event.rawEvent);
            }
        }

        switch (event.type) {
            case EventType::WindowClose:
                close_ = true;
                break;

            case EventType::WindowResize: {
                const auto& data = std::get<WindowResizeEventData>(event.data);
                handleWindowResize(data.width, data.height);
                break;
            }

            case EventType::WindowMove: {
                const auto& data = std::get<WindowMoveEventData>(event.data);
                handleWindowMoved(data.x, data.y);
                break;
            }

            case EventType::KeyDown:
            case EventType::KeyUp: {
                const auto& data = std::get<KeyEventData>(event.data);
                handleKeyEvent(event.type, data);
                break;
            }

            case EventType::MouseButtonDown:
            case EventType::MouseButtonUp: {
                const auto& data = std::get<MouseButtonEventData>(event.data);
                handleMouseButtonEvent(event.type, data);
                break;
            }

            case EventType::MouseMove: {
                const auto& data = std::get<MouseMoveEventData>(event.data);
                handleMouseMotionEvent(data);
                break;
            }

            case EventType::MouseWheel: {
                const auto& data = std::get<MouseWheelEventData>(event.data);
                handleMouseWheelEvent(data);
                break;
            }

            case EventType::FileDrop: {
                const auto& data = std::get<FileDropEventData>(event.data);
                handleDropEvent(data);
                break;
            }

            default:
                break;
        }
    }

    void handleWindowResize(int width, int height) {
        size_ = {width, height};
        for (const auto& listener : resizeListener) {
            listener(size_);
        }
    }

    void handleWindowMoved(int wx, int wy) {
        auto& wsi = getWindowSystemInterface();
        if (!wsi.getMonitorCount || !wsi.getMonitorSize) return;

        int numMonitors = wsi.getMonitorCount();
        // Simple implementation: estimate monitor based on window position
        for (int i = 0; i < numMonitors; ++i) {
            // Notify monitor change (more accurate implementation requires boundary info for each monitor)
            for (const auto& listener : monitorChangesListener) {
                listener(i);
                break;  // Notify only the first monitor
            }
        }
    }

    void handleKeyEvent(EventType type, const KeyEventData& keyEvent) {
        if (keyEvent.key == Key::ESCAPE && type == EventType::KeyDown && exitOnKeyEscape_) {
            close_ = true;
            return;
        }

        const KeyEvent evt{keyEvent.key, keyEvent.scancode, keyEvent.mods};

        if (type == EventType::KeyDown) {
            if (keyEvent.repeat) {
                scope.onKeyEvent(evt, KeyAction::REPEAT);
            } else {
                scope.onKeyEvent(evt, KeyAction::PRESS);
            }
        } else {
            scope.onKeyEvent(evt, KeyAction::RELEASE);
        }
    }

    void handleMouseButtonEvent(EventType type, const MouseButtonEventData& buttonEvent) {
        bool pressed = (type == EventType::MouseButtonDown);

        Vector2 mousePos(buttonEvent.x, buttonEvent.y);
        lastMousePos_.copy(mousePos);

        if (pressed) {
            scope.onMousePressedEvent(buttonEvent.button, mousePos, MouseAction::PRESS);
        } else {
            scope.onMousePressedEvent(buttonEvent.button, mousePos, MouseAction::RELEASE);
        }
    }

    void handleMouseMotionEvent(const MouseMoveEventData& motionEvent) {
        Vector2 mousePos(motionEvent.x, motionEvent.y);
        scope.onMouseMoveEvent(mousePos);
        lastMousePos_.copy(mousePos);
    }

    void handleMouseWheelEvent(const MouseWheelEventData& wheelEvent) {
        scope.onMouseWheelEvent({wheelEvent.xOffset, wheelEvent.yOffset});
    }

    void handleDropEvent(const FileDropEventData& dropEvent) {
        if (!dropEvent.paths.empty()) {
            scope.onDropEvent(dropEvent.paths);
        }
    }

    bool animateOnce(const std::function<void()>& f) {
        if (close_) {
            return false;
        }

        processEvents();

        if (close_) {
            return false;
        }

        f();

        auto& wsi = getWindowSystemInterface();
        if (wsi.swapBuffers) {
            wsi.swapBuffers(window);
        }

        return true;
    }

    void animate(const std::function<void()>& f) {
        while (animateOnce(f)) {}
    }

    void onWindowResize(std::function<void(WindowSize)> f) {
        this->resizeListener.emplace_back(std::move(f));
    }

    void onMonitorChange(std::function<void(int)> f) {
        this->monitorChangesListener.emplace_back(std::move(f));
    }

    void onRawEvent(std::function<void(const void *event)> f) {
        this->rawEventListeners.emplace_back(std::move(f));
    }

    void close() {
        close_ = true;
    }

    ~Impl() {
        auto& wsi = getWindowSystemInterface();
        if (wsi.destroyWindow && window) {
            wsi.destroyWindow(window);
        }
        if (wsi.shutdown) {
            wsi.shutdown();
        }
    }
};

Canvas::Canvas(const Parameters& params)
    : pimpl_(std::make_unique<Impl>(*this, params)) {}

Canvas::Canvas(const std::string& name)
    : Canvas(Parameters().title(name)) {}

Canvas::Canvas(const std::string& name, const std::unordered_map<std::string, ParameterValue>& values)
    : Canvas(Parameters(values).title(name)) {}


void Canvas::animate(const std::function<void()>& f) {
    pimpl_->animate(f);
}

bool Canvas::animateOnce(const std::function<void()>& f) {
    return pimpl_->animateOnce(f);
}

bool Canvas::isOpen() const {
    return !pimpl_->close_;
}

WindowSize Canvas::size() const {
    return pimpl_->getSize();
}

float Canvas::aspect() const {
    return size().aspect();
}

void Canvas::setSize(std::pair<int, int> size) {
    pimpl_->setSize(size);
}

void Canvas::onWindowResize(std::function<void(WindowSize)> f) {
    pimpl_->onWindowResize(std::move(f));
}

void Canvas::onMonitorChange(std::function<void(int)> f) const {
    pimpl_->onMonitorChange(std::move(f));
}

void Canvas::onRawEvent(std::function<void(const void *event)> f) const {
    pimpl_->onRawEvent(std::move(f));
}

void Canvas::close() {
    pimpl_->close();
}

void* Canvas::windowPtr() const {
    return pimpl_->window;
}

Canvas::~Canvas() = default;


Canvas::Parameters::Parameters() = default;

Canvas::Parameters::Parameters(const std::unordered_map<std::string, ParameterValue>& values) {

    std::vector<std::string> unused;
    for (const auto& [key, value] : values) {

        bool used = false;

        if (key == "antialiasing" || key == "aa") {
            antialiasing(std::get<int>(value));
            used = true;
        } else if (key == "vsync") {
            vsync(std::get<bool>(value));
            used = true;
        } else if (key == "resizable") {
            resizable(std::get<bool>(value));
            used = true;
        } else if (key == "size") {
            size(std::get<WindowSize>(value));
            used = true;
        } else if (key == "favicon") {
            auto path = std::get<std::string>(value);
            favicon(path);
            used = true;
        } else if (key == "exitOnKeyEscape") {
            exitOnKeyEscape(std::get<bool>(value));
            used = true;
        } else if (key == "headless") {
            headless(std::get<bool>(value));
            used = true;
        }

        if (!used) {
            unused.emplace_back(key);
        }
    }

    if (!unused.empty()) {
        std::cerr << "Unused Canvas parameters: [" << utils::join(unused, ',') << "]" << std::endl;
    }
}

Canvas::Parameters& Canvas::Parameters::title(std::string value) {
    this->title_ = std::move(value);
    return *this;
}

Canvas::Parameters& Canvas::Parameters::size(WindowSize size) {
    this->size_ = size;
    return *this;
}

Canvas::Parameters& Canvas::Parameters::size(int width, int height) {
    return this->size({width, height});
}

Canvas::Parameters& Canvas::Parameters::antialiasing(int antialiasing) {
    this->antialiasing_ = antialiasing;
    return *this;
}

Canvas::Parameters& Canvas::Parameters::vsync(bool flag) {
    this->vsync_ = flag;
    return *this;
}

Canvas::Parameters& Canvas::Parameters::resizable(bool flag) {
    this->resizable_ = flag;
    return *this;
}

Canvas::Parameters& Canvas::Parameters::favicon(const std::filesystem::path& path) {
    if (exists(path)) {
        favicon_ = path;
    } else {
        std::cerr << "Invalid favicon path: " << absolute(path) << std::endl;
    }
    return *this;
}

Canvas::Parameters& Canvas::Parameters::exitOnKeyEscape(bool flag) {
    exitOnKeyEscape_ = flag;
    return *this;
}

Canvas::Parameters& Canvas::Parameters::headless(bool flag) {
    headless_ = flag;
    return *this;
}


//=====================================================================
// Monitor-related Functions
//=====================================================================

WindowSize monitor::monitorSize(int monitor) {
    if (!hasWindowSystemInterface()) {
        return {800, 600};  // fallback
    }

    auto& wsi = getWindowSystemInterface();
    if (wsi.getMonitorSize) {
        return wsi.getMonitorSize(monitor);
    }
    return {800, 600};  // fallback
}

std::pair<float, float> monitor::contentScale(int monitor) {
    if (!hasWindowSystemInterface()) {
        return {1.0f, 1.0f};  // fallback
    }

    auto& wsi = getWindowSystemInterface();
    if (wsi.getMonitorContentScale) {
        return wsi.getMonitorContentScale(monitor);
    }
    return {1.0f, 1.0f};  // fallback
}


//=====================================================================
// Generic Interface Global Management
//=====================================================================

namespace threepp::generic {

    namespace {
        std::optional<WindowSystemInterface> g_windowSystemInterface;
        std::optional<ImGuiBackendInterface> g_imguiBackendInterface;
    }

    void setWindowSystemInterface(const WindowSystemInterface& iface) {
        g_windowSystemInterface = iface;
    }

    WindowSystemInterface& getWindowSystemInterface() {
        if (!g_windowSystemInterface) {
            throw std::runtime_error("WindowSystemInterface not set. Call setWindowSystemInterface() first.");
        }
        return *g_windowSystemInterface;
    }

    bool hasWindowSystemInterface() {
        return g_windowSystemInterface.has_value();
    }

    void setImGuiBackendInterface(const ImGuiBackendInterface& iface) {
        g_imguiBackendInterface = iface;
    }

    ImGuiBackendInterface& getImGuiBackendInterface() {
        if (!g_imguiBackendInterface) {
            throw std::runtime_error("ImGuiBackendInterface not set. Call setImGuiBackendInterface() first.");
        }
        return *g_imguiBackendInterface;
    }

    bool hasImGuiBackendInterface() {
        return g_imguiBackendInterface.has_value();
    }

}// namespace threepp::generic


//=====================================================================
// ImGui Wrapper Functions (for extern reference)
//=====================================================================

extern "C" {

    bool threepp_generic_imgui_init_for_opengl(void* window) {
        if (!threepp::generic::hasImGuiBackendInterface()) {
            return false;
        }
        auto& iface = threepp::generic::getImGuiBackendInterface();
        if (iface.initForOpenGL) {
            return iface.initForOpenGL(window);
        }
        return false;
    }

    void threepp_generic_imgui_process_event(const threepp::generic::Event* event) {
        if (!threepp::generic::hasImGuiBackendInterface() || !event) {
            return;
        }
        auto& iface = threepp::generic::getImGuiBackendInterface();
        if (iface.processEvent) {
            iface.processEvent(*event);
        }
    }

    void threepp_generic_imgui_process_raw_event(const void* rawEvent) {
        if (!threepp::generic::hasImGuiBackendInterface() || !rawEvent) {
            return;
        }
        auto& iface = threepp::generic::getImGuiBackendInterface();
        if (iface.processEvent) {
            threepp::generic::Event genericEvent;
            genericEvent.rawEvent = rawEvent;
            iface.processEvent(genericEvent);
        }
    }

    bool threepp_generic_imgui_should_install_event_handler() {
        if (!threepp::generic::hasImGuiBackendInterface()) {
            return false;
        }
        auto& iface = threepp::generic::getImGuiBackendInterface();
        return iface.processEvent != nullptr;
    }

    void threepp_generic_imgui_new_frame() {
        if (!threepp::generic::hasImGuiBackendInterface()) {
            return;
        }
        auto& iface = threepp::generic::getImGuiBackendInterface();
        if (iface.newFrame) {
            iface.newFrame();
        }
    }

    void threepp_generic_imgui_shutdown() {
        if (!threepp::generic::hasImGuiBackendInterface()) {
            return;
        }
        auto& iface = threepp::generic::getImGuiBackendInterface();
        if (iface.shutdown) {
            iface.shutdown();
        }
    }

}
