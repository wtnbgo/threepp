
#ifndef THREEPP_LOAD_GLAD_HPP
#define THREEPP_LOAD_GLAD_HPP

#include <glad/glad.h>


namespace threepp {

    // Set a custom GL function loader (e.g., ANGLE eglGetProcAddress via the host).
    // Signature is void*(*)(const char*) so the plugin側 (main.cpp) が同型で
    // 前方宣言でき、glad2 の GLADloadfunc へは loadGlad() 内でキャストする。
    // This must be called before loadGlad().
    void initGlad(void *(*procAddress)(const char *));

    // Load OpenGL ES functions using glad2 (gladLoadGLES2) with the loader set
    // by initGlad(). ホスト提供の GLES3 コンテキストへ結線する前提。
    void loadGlad();
}

#endif//THREEPP_LOAD_GLAD_HPP
