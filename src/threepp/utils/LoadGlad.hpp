
#ifndef THREEPP_LOAD_GLAD_HPP
#define THREEPP_LOAD_GLAD_HPP

#include <glad/glad.h>


namespace threepp {

    // Set a custom GL function loader (e.g., glfwGetProcAddress, SDL_GL_GetProcAddress)
    // This should be called before loadGlad() if you want to use a custom loader
    void initGlad(GLADloadproc procAddress);

    // Load OpenGL functions using glad
    // If initGlad() was called with a custom loader, uses gladLoadGLLoader()
    // Otherwise, uses gladLoadGL()
    void loadGlad();
}

#endif//THREEPP_LOAD_GLAD_HPP
