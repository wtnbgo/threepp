#ifndef THREEPP_GLTFLOADER_HPP
#define THREEPP_GLTFLOADER_HPP

#include "threepp/animation/AnimationClip.hpp"
#include "threepp/loaders/MaterialVariants.hpp"
#include "threepp/threepp.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace threepp {

    class Skeleton;// only held as shared_ptr in GLTFResult; not pulled in by threepp.hpp

    struct GLTFResult {
        std::shared_ptr<Group> scene;                          ///< Root node of the loaded model
        std::vector<std::shared_ptr<Group>> scenes;            ///< All scenes in the file
        std::vector<std::shared_ptr<AnimationClip>> animations;///< All animations in the file
        MaterialVariants variants;                             ///< Named material variants (empty if none)

        // --- Extension layer support (e.g. VRM) -----------------------------
        // Populated for every load. Standard glTF users can ignore these.
        // The raw glTF JSON is exposed as text (not nlohmann::json) so that this
        // public header does not depend on nlohmann; consumers that need the
        // extensions/extras re-parse it with their own JSON library. This is a
        // load-time operation, so the extra parse is negligible.

        std::string json;                                      ///< Raw glTF JSON text (extensions/extras included); empty if unset

        /// glTF index -> constructed threepp object (three.js parser.associations equivalent)
        std::unordered_map<int, std::shared_ptr<Object3D>> nodes;     ///< node index -> Object3D/Bone/Group
        std::unordered_map<int, std::shared_ptr<Material>> materials; ///< material index -> Material
        std::unordered_map<int, std::shared_ptr<Texture>> textures;   ///< texture index -> Texture
        std::unordered_map<int, std::shared_ptr<Skeleton>> skins;     ///< skin index -> Skeleton
        std::map<std::pair<int, int>, std::shared_ptr<Mesh>> meshPrimitives; ///< (mesh index, primitive index) -> Mesh
    };

    class GLTFLoader {
    public:
        std::optional<GLTFResult> load(const std::filesystem::path& path);

    private:
        struct Impl;
    };

} // namespace threepp

#endif
