
#ifndef THREEPP_GLMORPHTARGETS_HPP
#define THREEPP_GLMORPHTARGETS_HPP

#include "GLCapabilities.hpp"
#include "GLProgram.hpp"
#include "GLUniforms.hpp"
#include "threepp/core/BufferGeometry.hpp"
#include "threepp/materials/materials.hpp"
#include "threepp/objects/ObjectWithMorphTargetInfluences.hpp"
#include "threepp/textures/DataTexture.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace threepp::gl {

    // モーフターゲット (blend shape) をテクスチャに格納して適用する。
    //
    // 旧実装は影響度上位 8 個を頂点 attribute (morphTarget0..7) に流し込む方式で、
    // 1 メッシュあたり同時 8 モーフが上限だった。本実装は全モーフの位置(・法線)差分を
    // 1 枚の RGBA32F テクスチャに詰め、頂点シェーダが gl_VertexID + texelFetch で全モーフを
    // 走査・加重する。同時モーフ数の実質的な上限が無くなる (GLES 300es / GL 330 前提)。
    class GLMorphTargets {

    public:
        struct Entry {
            std::shared_ptr<DataTexture> texture;
            int width{};
            int texelsPerMorph{};// = 頂点数 * stride
            int count{};         // モーフ数
            int stride{};        // 頂点あたり texel 数 (1=位置のみ / 2=位置+法線)
        };

        std::unordered_map<unsigned int, Entry> entries;

        void update(Object3D* object, BufferGeometry* geometry, Material* material, GLProgram* program, GLTextures* textures) {

            auto morphMat = material->as<MaterialWithMorphTargets>();
            if (!morphMat) return;

            auto morphPosition = geometry->getMorphAttribute("position");
            if (!morphPosition || morphPosition->empty()) return;

            const int morphCount = static_cast<int>(morphPosition->size());
            const bool useNormals = morphMat->morphNormals;
            const int stride = useNormals ? 2 : 1;

            std::vector<std::shared_ptr<BufferAttribute>>* morphNormal = nullptr;
            if (useNormals) morphNormal = geometry->getMorphAttribute("normal");

            auto& entry = entries[geometry->id];
            if (!entry.texture || entry.count != morphCount || entry.stride != stride) {

                buildTexture(entry, *morphPosition, morphNormal, morphCount, stride);
            }

            // 影響度 (毎フレーム変わる) を集める
            std::vector<float> influences(morphCount, 0.f);
            if (auto owm = dynamic_cast<ObjectWithMorphTargetInfluences*>(object)) {
                auto& objInf = owm->morphTargetInfluences();
                const int n = std::min(morphCount, static_cast<int>(objInf.size()));
                for (int i = 0; i < n; ++i) influences[i] = objInf[i];
            }

            float sum = 0.f;
            for (float f : influences) sum += f;
            const float baseInfluence = geometry->morphTargetsRelative ? 1.f : 1.f - sum;

            auto uniforms = program->getUniforms();
            uniforms->setValue("morphTargetBaseInfluence", baseInfluence);
            uniforms->setValue("morphTargetInfluences", influences);
            uniforms->setValue("morphTargetsTexture", static_cast<Texture*>(entry.texture.get()), textures);
            uniforms->setValue("morphTargetsTextureWidth", entry.width);
            uniforms->setValue("morphTargetsTexelsPerMorph", entry.texelsPerMorph);
        }

    private:
        static void buildTexture(Entry& entry,
                                 const std::vector<std::shared_ptr<BufferAttribute>>& morphPosition,
                                 const std::vector<std::shared_ptr<BufferAttribute>>* morphNormal,
                                 int morphCount, int stride) {

            const int numVertices = morphPosition[0]->count();
            const long long totalTexels = static_cast<long long>(morphCount) * numVertices * stride;

            const int maxSize = GLCapabilities::instance().maxTextureSize;
            int width = static_cast<int>(std::min<long long>(totalTexels, maxSize));
            if (width < 1) width = 1;
            int height = static_cast<int>((totalTexels + width - 1) / width);
            if (height < 1) height = 1;

            std::vector<float> buffer(static_cast<size_t>(width) * height * 4, 0.f);

            for (int m = 0; m < morphCount; ++m) {

                auto posAttr = std::dynamic_pointer_cast<TypedBufferAttribute<float>>(morphPosition[m]);
                std::shared_ptr<TypedBufferAttribute<float>> nrmAttr;
                if (stride == 2 && morphNormal && m < static_cast<int>(morphNormal->size()))
                    nrmAttr = std::dynamic_pointer_cast<TypedBufferAttribute<float>>((*morphNormal)[m]);

                for (int v = 0; v < numVertices; ++v) {

                    const long long base = (static_cast<long long>(m) * numVertices + v) * stride;

                    if (posAttr) {
                        const size_t pi = static_cast<size_t>(base + 0) * 4;
                        buffer[pi + 0] = posAttr->getX(v);
                        buffer[pi + 1] = posAttr->getY(v);
                        buffer[pi + 2] = posAttr->getZ(v);
                    }
                    if (stride == 2 && nrmAttr) {
                        const size_t ni = static_cast<size_t>(base + 1) * 4;
                        buffer[ni + 0] = nrmAttr->getX(v);
                        buffer[ni + 1] = nrmAttr->getY(v);
                        buffer[ni + 2] = nrmAttr->getZ(v);
                    }
                }
            }

            auto tex = DataTexture::create(ImageData(std::move(buffer)),
                                           static_cast<unsigned int>(width), static_cast<unsigned int>(height));
            tex->format = Format::RGBA;
            tex->type = Type::Float;
            tex->magFilter = Filter::Nearest;
            tex->minFilter = Filter::Nearest;
            tex->wrapS = TextureWrapping::ClampToEdge;
            tex->wrapT = TextureWrapping::ClampToEdge;
            tex->generateMipmaps = false;
            tex->unpackAlignment = 1;
            tex->needsUpdate();

            entry.texture = tex;
            entry.width = width;
            entry.texelsPerMorph = numVertices * stride;
            entry.count = morphCount;
            entry.stride = stride;
        }
    };

}// namespace threepp::gl

#endif//THREEPP_GLMORPHTARGETS_HPP
