// https://github.com/mrdoob/three.js/blob/r129/src/renderers/WebGLRenderer.js

#ifndef THREEPP_GLRENDERER_HPP
#define THREEPP_GLRENDERER_HPP

#include "threepp/renderers/Renderer.hpp"

#include "threepp/renderers/gl/GLInfo.hpp"
#include "threepp/renderers/gl/GLShadowMap.hpp"
#include "threepp/renderers/gl/GLState.hpp"

namespace threepp {

    class Camera;
    class Canvas;
    class Scene;
    class BufferGeometry;
    class Object3D;
    class Material;
    class Texture;
    class RenderTarget;
    class BufferAttribute;

    class GLRenderer : public Renderer {

    public:
        struct Parameters {

            // bool alpha;
            // bool depth;
            bool premultipliedAlpha;
        };

        /// Canvas-aware constructor: initialises the OpenGL window on `canvas`
        /// and derives the viewport size from it. Preferred over the size-only
        /// constructor when using Canvas, as it handles lazy window init.
        explicit GLRenderer(Canvas& canvas, const Parameters& parameters = {});

        /// Headless / size-only constructor (fork addition): does NOT create or
        /// initialise a window — it assumes an OpenGL context is already current
        /// (e.g. provided by the host application, as with the Kirikiri plugin).
        /// Restores the pre-2026 threepp API that upstream replaced with the
        /// Canvas-aware constructor above.
        explicit GLRenderer(std::pair<int, int> size, const Parameters& parameters = {});

        GLRenderer(GLRenderer&&) = delete;
        GLRenderer(const GLRenderer&) = delete;
        GLRenderer& operator=(const GLRenderer&) = delete;
        GLRenderer& operator=(GLRenderer&&) = delete;

        // --- GL-specific accessors (not on Renderer base) ---

        const gl::GLInfo& info();

        gl::GLShadowMap& shadowMap() override;

        [[nodiscard]] const gl::GLShadowMap& shadowMap() const override;

        gl::GLState& state();

        [[nodiscard]] std::optional<unsigned int> getGlTextureId(Texture& texture) const;

        [[nodiscard]] std::optional<unsigned int> getGlBufferId(BufferAttribute& bufferAttribute) const;

        // --- Renderer interface overrides ---

        [[nodiscard]] float getTargetPixelRatio() const override;

        void setPixelRatio(float value) override;

        [[nodiscard]] WindowSize size() const override;

        void setSize(const std::pair<int, int>& size) override;

        void setViewport(const Vector4& v) override;

        void setViewport(int x, int y, int width, int height) override;

        void setScissor(const Vector4& v) override;

        void setScissor(int x, int y, int width, int height) override;

        void setScissorTest(bool boolean) override;

        void setClearColor(const Color& color, float alpha = 1) override;

        void clear(bool color = true, bool depth = true, bool stencil = true) override;

        void render(Object3D& scene, Camera& camera) override;

        RenderTarget* getRenderTarget() override;

        void setRenderTarget(RenderTarget* renderTarget, int activeCubeFace = 0, int activeMipmapLevel = 0) override;

        [[nodiscard]] std::vector<unsigned char> readRGBPixels() override;

        void dispose() override;

        // --- Additional GLRenderer-specific methods ---

        void getDrawingBufferSize(Vector2& target) const;

        void setDrawingBufferSize(const std::pair<int, int>& size, int pixelRatio);

        void getCurrentViewport(Vector4& target) const;

        void getViewport(Vector4& target) const;

        void setViewport(const std::pair<int, int>& pos, const std::pair<int ,int>& size);

        void getScissor(Vector4& target);

        void setScissor(const std::pair<int, int>& pos, const std::pair<int, int>& size);

        [[nodiscard]] bool getScissorTest() const;

        void getClearColor(Color& target) const override;

        [[nodiscard]] float getClearAlpha() const override;

        void setClearAlpha(float clearAlpha) override;

        void clearColor() override;
        void clearDepth() override;
        void clearStencil() override;

        void renderBufferDirect(Camera* camera, Scene* scene, BufferGeometry* geometry, Material* material, Object3D* object, std::optional<GeometryGroup> group);

        [[nodiscard]] int getActiveCubeFace() const;

        [[nodiscard]] int getActiveMipmapLevel() const;

        void copyFramebufferToTexture(const Vector2& position, Texture& texture, int level = 0) override;

        void readPixels(const Vector2& position, const std::pair<int, int>& size, Format format, unsigned char* data);

        // Experimental threepp function
        void copyTextureToImage(Texture& texture) override;

        void setDepthMask(bool flag) override;

        void resetState();

        // 画面ターゲット (renderTarget=nullptr) 描画時に bind するフレームバッファ ID。
        // 既定は 0 (ウィンドウのデフォルト FBO)。ホストが中間 FBO を提供して読み戻す
        // 構成 (例: 吉里吉里 GLESAdaptor.capture) では、その FBO の ID を設定することで
        // threepp の描画をホストの捕捉先 FBO へ向けられる。
        void setDefaultFramebuffer(unsigned int framebuffer);
        [[nodiscard]] unsigned int getDefaultFramebuffer() const;
        // 現在 GL に bind されている FBO をデフォルトフレームバッファとして取り込む。
        // ホストが capture 用 FBO を bind した状態で呼ぶ (例: onBeginScene の先頭)。
        void setDefaultFramebufferToCurrent();

        // MSAA アンチエイリアス。ホストが単一サンプルの捕捉 FBO しか用意しない構成
        // (吉里吉里 GLESAdaptor 等) 向け。sampleCount>1 のとき beginFrame() が内部の
        // マルチサンプル FBO を bind してそこへ描き、endFrame() でホスト FBO へ resolve
        // (blit) する。sampleCount<=1 なら従来どおりホスト FBO へ直接描く。
        // 使い方: 描画コールバック内で beginFrame(w,h) → resetState/render → endFrame()。
        void setSampleCount(int samples);
        [[nodiscard]] int sampleCount() const;
        void beginFrame(int width, int height);
        void endFrame();

        [[nodiscard]] const gl::GLInfo& info() const;

        void writeFramebuffer(const std::filesystem::path& filename) override;

        ~GLRenderer() override;

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace threepp

#endif//THREEPP_GLRENDERER_HPP
