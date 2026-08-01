

#include "threepp/utils/LoadGlad.hpp"

#include <iostream>

namespace {
    // ホスト(ANGLE)から受け取る getProcAddress。void*(*)(const char*) で保持し、
    // glad2 の gladLoadGLES2 へ渡すときに GLADloadfunc へキャストする。
    void *(*g_loader)(const char *) = nullptr;
}

void threepp::initGlad(void *(*procAddress)(const char *)) {
    g_loader = procAddress;
}

void threepp::loadGlad() {

    static bool gladInitialized = false;

    if (!gladInitialized) {
        if (!g_loader) {
            std::cerr << "threepp::loadGlad: GL loader not set (call initGlad first)" << std::endl;
            exit(EXIT_FAILURE);
        }
        // glad2 GLES2 ローダ。GLADloadfunc は GLADapiproc(*)(const char*)。
        if (!gladLoadGLES2(reinterpret_cast<GLADloadfunc>(g_loader))) {
            std::cerr << "Failed to initialize GLAD (GLES2)" << std::endl;
            exit(EXIT_FAILURE);
        }
        gladInitialized = true;
    }
}
