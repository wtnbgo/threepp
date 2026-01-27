

#include "threepp/utils/LoadGlad.hpp"

#include <iostream>

namespace {
    GLADloadproc g_gladLoadProc = nullptr;
}

void threepp::initGlad(GLADloadproc procAddress) {
    g_gladLoadProc = procAddress;
}

void threepp::loadGlad() {

    static bool gladInitialized = false;

    if (!gladInitialized) {
        int result;
        if (g_gladLoadProc) {
            result = gladLoadGLLoader(g_gladLoadProc);
        } else {
            result = gladLoadGL();
        }
        
        if (!result) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            exit(EXIT_FAILURE);
        }
        gladInitialized = true;
    }
}
