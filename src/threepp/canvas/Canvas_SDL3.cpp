
#include "threepp/canvas/Canvas.hpp"
#include "threepp/canvas/Monitor.hpp"

#include "threepp/favicon.hpp"
#include "threepp/loaders/ImageLoader.hpp"
#include "threepp/utils/StringUtils.hpp"
#include "threepp/utils/LoadGlad.hpp"

#include <SDL3/SDL.h>

#include <iostream>
#include <optional>

using namespace threepp;

namespace {

    void setWindowIcon(SDL_Window* window, std::optional<std::filesystem::path> customIcon) {

        ImageLoader imageLoader;
        std::optional<Image> favicon;
        if (customIcon) {
            favicon = imageLoader.load(*customIcon, 4, false);
        } else {
            favicon = imageLoader.load(faviconSource(), 4, false);
        }
        if (favicon) {
            SDL_Surface* surface = SDL_CreateSurfaceFrom(
                static_cast<int>(favicon->width),
                static_cast<int>(favicon->height),
                SDL_PIXELFORMAT_RGBA32,
                favicon->data().data(),
                static_cast<int>(favicon->width * 4)
            );
            if (surface) {
                SDL_SetWindowIcon(window, surface);
                SDL_DestroySurface(surface);
            }
        }
    }

    Key sdlKeyCodeToKey(SDL_Keycode keyCode) {

        // clang-format off
        switch (keyCode) {
            case SDLK_0: return Key::NUM_0;
            case SDLK_1: return Key::NUM_1;
            case SDLK_2: return Key::NUM_2;
            case SDLK_3: return Key::NUM_3;
            case SDLK_4: return Key::NUM_4;
            case SDLK_5: return Key::NUM_5;
            case SDLK_6: return Key::NUM_6;
            case SDLK_7: return Key::NUM_7;
            case SDLK_8: return Key::NUM_8;
            case SDLK_9: return Key::NUM_9;

            case SDLK_F1: return Key::F1;
            case SDLK_F2: return Key::F2;
            case SDLK_F3: return Key::F3;
            case SDLK_F4: return Key::F4;
            case SDLK_F5: return Key::F5;
            case SDLK_F6: return Key::F6;
            case SDLK_F7: return Key::F7;
            case SDLK_F8: return Key::F8;
            case SDLK_F9: return Key::F9;
            case SDLK_F10: return Key::F10;
            case SDLK_F11: return Key::F11;
            case SDLK_F12: return Key::F12;

            case SDLK_A: return Key::A;
            case SDLK_B: return Key::B;
            case SDLK_C: return Key::C;
            case SDLK_D: return Key::D;
            case SDLK_E: return Key::E;
            case SDLK_F: return Key::F;
            case SDLK_G: return Key::G;
            case SDLK_H: return Key::H;
            case SDLK_J: return Key::J;
            case SDLK_K: return Key::K;
            case SDLK_L: return Key::L;
            case SDLK_M: return Key::M;
            case SDLK_N: return Key::N;
            case SDLK_O: return Key::O;
            case SDLK_P: return Key::P;
            case SDLK_Q: return Key::Q;
            case SDLK_R: return Key::R;
            case SDLK_S: return Key::S;
            case SDLK_T: return Key::T;
            case SDLK_U: return Key::U;
            case SDLK_V: return Key::V;
            case SDLK_W: return Key::W;
            case SDLK_X: return Key::X;
            case SDLK_Y: return Key::Y;
            case SDLK_Z: return Key::Z;

            case SDLK_UP: return Key::UP;
            case SDLK_DOWN: return Key::DOWN;
            case SDLK_LEFT: return Key::LEFT;
            case SDLK_RIGHT: return Key::RIGHT;

            case SDLK_SPACE: return Key::SPACE;
            case SDLK_COMMA: return Key::COMMA;
            case SDLK_MINUS: return Key::MINUS;
            case SDLK_PERIOD: return Key::PERIOD;
            case SDLK_SLASH: return Key::SLASH;

            case SDLK_RETURN: return Key::ENTER;
            case SDLK_TAB: return Key::TAB;
            case SDLK_BACKSPACE: return Key::BACKSPACE;
            case SDLK_INSERT: return Key::INSERT;
            case SDLK_DELETE: return Key::DEL;

            default: return Key::UNKNOWN;

        }
        // clang-format on
    }

    int sdlMouseButtonToIndex(Uint8 button) {
        switch (button) {
            case SDL_BUTTON_LEFT: return 0;
            case SDL_BUTTON_MIDDLE: return 1;
            case SDL_BUTTON_RIGHT: return 2;
            default: return button;
        }
    }

    SDL_Keymod getKeyMods(SDL_Keymod mod) {
        int mods = 0;
        if (mod & SDL_KMOD_SHIFT) mods |= 1;
        if (mod & SDL_KMOD_CTRL) mods |= 2;
        if (mod & SDL_KMOD_ALT) mods |= 4;
        if (mod & SDL_KMOD_GUI) mods |= 8;
        return static_cast<SDL_Keymod>(mods);
    }

    bool sdlInitialized = false;

    void initSDL() {
        if (!sdlInitialized) {
            sdlInitialized = true;

            if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
                std::cerr << "Error: SDL_Init failed: " << SDL_GetError() << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }

}// namespace

struct Canvas::Impl {

    Canvas& scope;
    SDL_Window* window;
    SDL_GLContext glContext;

    WindowSize size_;
    Vector2 lastMousePos_;

    bool close_{false};
    bool exitOnKeyEscape_;

    std::vector<std::function<void(WindowSize)>> resizeListener;
    std::vector<std::function<void(int monitor)>> monitorChangesListener;
    std::vector<std::function<void(const void *event)>> rawEventListeners;

    explicit Impl(Canvas& scope, const Parameters& params)
        : scope(scope), exitOnKeyEscape_(params.exitOnKeyEscape_) {

        initSDL();

        if (params.size_) {
            size_ = *params.size_;
        } else {
            const auto fullSize = monitor::monitorSize();
            size_ = {fullSize.width() / 2, fullSize.height() / 2};
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

        if (params.antialiasing_ > 0) {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, params.antialiasing_);
        }

        SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL;
        if (params.resizable_) {
            windowFlags |= SDL_WINDOW_RESIZABLE;
        }
        if (params.headless_) {
            windowFlags |= SDL_WINDOW_HIDDEN;
        }

        window = SDL_CreateWindow(params.title_.c_str(), size_.width(), size_.height(), windowFlags);
        if (!window) {
            std::cerr << "Error: SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            exit(EXIT_FAILURE);
        }

        glContext = SDL_GL_CreateContext(window);
        if (!glContext) {
            std::cerr << "Error: SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            exit(EXIT_FAILURE);
        }

        SDL_GL_MakeCurrent(window, glContext);

        setWindowIcon(window, params.favicon_);
        initGlad(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress));
        loadGlad();
        SDL_GL_SetSwapInterval(params.vsync_ ? 1 : 0);

        if (params.antialiasing_ > 0) {
            glEnable(GL_MULTISAMPLE);
        }

        glEnable(GL_PROGRAM_POINT_SIZE);
    }

    [[nodiscard]] const WindowSize& getSize() const {

        return size_;
    }

    void setSize(std::pair<int, int> size) const {

        SDL_SetWindowSize(window, size.first, size.second);
    }

    void processEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }
    }

    void handleEvent(const SDL_Event& event) {
        // Forward raw event to listeners (e.g., ImGUI)
        for (const auto& listener : rawEventListeners) {
            listener((void*)&event);
        }

        switch (event.type) {
            case SDL_EVENT_QUIT:
                close_ = true;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                handleWindowResize(event.window.data1, event.window.data2);
                break;

            case SDL_EVENT_WINDOW_MOVED:
                handleWindowMoved(event.window.data1, event.window.data2);
                break;

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                handleKeyEvent(event.key);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                handleMouseButtonEvent(event.button);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                handleMouseMotionEvent(event.motion);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                handleMouseWheelEvent(event.wheel);
                break;

            case SDL_EVENT_DROP_FILE:
                handleDropEvent(event.drop);
                break;

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
        int numDisplays = 0;
        SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
        if (!displays) return;

        for (int i = 0; i < numDisplays; ++i) {
            SDL_Rect bounds;
            if (SDL_GetDisplayBounds(displays[i], &bounds)) {
                if (wx >= bounds.x && wx < bounds.x + bounds.w &&
                    wy >= bounds.y && wy < bounds.y + bounds.h) {
                    for (const auto& listener : monitorChangesListener) {
                        listener(i);
                    }
                    break;
                }
            }
        }
        SDL_free(displays);
    }

    void handleKeyEvent(const SDL_KeyboardEvent& keyEvent) {
        SDL_Keycode keyCode = keyEvent.key;
        bool pressed = (keyEvent.type == SDL_EVENT_KEY_DOWN);
        bool repeat = keyEvent.repeat;

        if (keyCode == SDLK_ESCAPE && pressed && exitOnKeyEscape_) {
            close_ = true;
            return;
        }

        SDL_Keymod mod = getKeyMods(keyEvent.mod);
        const KeyEvent evt{sdlKeyCodeToKey(keyCode), static_cast<int>(keyEvent.scancode), static_cast<int>(mod)};

        if (pressed) {
            if (repeat) {
                scope.onKeyEvent(evt, KeyAction::REPEAT);
            } else {
                scope.onKeyEvent(evt, KeyAction::PRESS);
            }
        } else {
            scope.onKeyEvent(evt, KeyAction::RELEASE);
        }
    }

    void handleMouseButtonEvent(const SDL_MouseButtonEvent& buttonEvent) {
        bool pressed = (buttonEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        int button = sdlMouseButtonToIndex(buttonEvent.button);

        Vector2 mousePos(buttonEvent.x, buttonEvent.y);
        lastMousePos_.copy(mousePos);

        if (pressed) {
            scope.onMousePressedEvent(button, mousePos, MouseAction::PRESS);
        } else {
            scope.onMousePressedEvent(button, mousePos, MouseAction::RELEASE);
        }
    }

    void handleMouseMotionEvent(const SDL_MouseMotionEvent& motionEvent) {
        Vector2 mousePos(motionEvent.x, motionEvent.y);
        scope.onMouseMoveEvent(mousePos);
        lastMousePos_.copy(mousePos);
    }

    void handleMouseWheelEvent(const SDL_MouseWheelEvent& wheelEvent) {
        float xOffset = wheelEvent.x;
        float yOffset = wheelEvent.y;

        // SDL3 wheel direction is already natural (positive = scroll up)
        scope.onMouseWheelEvent({xOffset, yOffset});
    }

    void handleDropEvent(const SDL_DropEvent& dropEvent) {
        std::vector<std::string> paths;
        if (dropEvent.data) {
            paths.emplace_back(dropEvent.data);
        }
        if (!paths.empty()) {
            scope.onDropEvent(paths);
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

        SDL_GL_SwapWindow(window);

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
        if (glContext) {
            SDL_GL_DestroyContext(glContext);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
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


WindowSize monitor::monitorSize(int monitor) {

    initSDL();

    int numDisplays = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
    if (!displays || monitor >= numDisplays) {
        SDL_free(displays);
        return {800, 600}; // fallback
    }

    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displays[monitor]);
    SDL_free(displays);

    if (mode) {
        return {mode->w, mode->h};
    }

    return {800, 600}; // fallback
}

std::pair<float, float> monitor::contentScale(int monitor) {

    initSDL();

    int numDisplays = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
    if (!displays || monitor >= numDisplays) {
        SDL_free(displays);
        return {1.0f, 1.0f}; // fallback
    }

    float scale = SDL_GetDisplayContentScale(displays[monitor]);
    SDL_free(displays);

    return {scale, scale};
}
