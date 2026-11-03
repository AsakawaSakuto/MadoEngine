#include "ModelCache.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {

using Json = nlohmann::json;
namespace fs = std::filesystem;
constexpr std::array<char, 8> kMagic = { 'M', 'A', 'D', 'O', 'M', 'D', 'L', '\0' };
constexpr std::uint32_t kVersion = 1;
constexpr std::uint64_t kHeaderSize = 28;
constexpr std::uint64_t kMaximumPayload = 512ULL * 1024 * 1024;
constexpr std::uint64_t kHashBasis = 14695981039346656037ULL;
constexpr std::uint64_t kHashPrime = 1099511628211ULL;
static_assert(std::endian::native == std::endian::little && sizeof(float) == 4);
static_assert(sizeof(ModelVertexData) == 48 && sizeof(ModelSubMesh) == 12);
static_assert(sizeof(Vector3) == 12 && sizeof(Vector4) == 16 && sizeof(Matrix4x4) == 64);
static_assert(sizeof(QuaternionTransform) == 40 && sizeof(VertexWeightData) == 8);
static_assert(sizeof(KeyframeVector3) == 16 && sizeof(KeyframeQuaternion) == 20);

/// @brief 配布先でも復元できるUTF-8のパス表現を生成
/// @param path 変換するパス
/// @return UTF-8のパス文字列
std::string EncodePath(const fs::path& path) {
    const auto value = path.generic_u8string();
    return { value.begin(), value.end() };
}

/// @brief UTF-8の保存パスを復元
/// @param value UTF-8のパス文字列
/// @return 復元したパス
fs::path DecodePath(const std::string& value) {
    return fs::path(std::u8string(value.begin(), value.end()));
}

/// @brief バイト列の破損検出用識別値を更新
/// @param hash 更新対象の識別値
/// @param bytes バイト列の先頭
/// @param size バイト数
void HashBytes(std::uint64_t& hash, const std::uint8_t* bytes, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= kHashPrime;
    }
}

/// @brief ポインターを含まない配列をバイナリとして保存
/// @param values 保存する配列の先頭
/// @param count 要素数
/// @return バイナリ表現
template <typename T>
Json EncodeArray(const T* values, std::size_t count) {
    static_assert(std::is_trivially_copyable_v<T>);
    std::vector<std::uint8_t> bytes(count * sizeof(T));
    if (!bytes.empty()) {
        std::memcpy(bytes.data(), values, bytes.size());
    }
    return Json::binary(std::move(bytes));
}

/// @brief 数学データをバイナリとして保存
/// @param value 保存する値
/// @return バイナリ表現
template <typename T>
Json EncodeValue(const T& value) {
    return EncodeArray(&value, 1);
}

/// @brief 要素境界を検証してバイナリ配列を復元
/// @param json バイナリ表現
/// @return 復元した配列
template <typename T>
std::vector<T> DecodeArray(const Json& json) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto& bytes = json.get_binary();
    if (bytes.size() % sizeof(T) != 0) {
        throw std::runtime_error("Modelキャッシュの配列サイズが不正");
    }
    std::vector<T> values(bytes.size() / sizeof(T));
    if (!bytes.empty()) {
        std::memcpy(values.data(), bytes.data(), bytes.size());
    }
    return values;
}

/// @brief 単一要素の数学データを復元
/// @param json バイナリ表現
/// @return 復元した値
template <typename T>
T DecodeValue(const Json& json) {
    const auto values = DecodeArray<T>(json);
    if (values.size() != 1) {
        throw std::runtime_error("Modelキャッシュの要素数が不正");
    }
    return values.front();
}

/// @brief Nodeの階層と変換結果を保存
/// @param node 保存するNode
/// @return Nodeの保存表現
Json EncodeNode(const ModelNode& node) {
    Json children = Json::array();
    for (const auto& child : node.children) {
        children.push_back(EncodeNode(child));
    }
    return { { "name", node.name }, { "transform", EncodeValue(node.transform) },
        { "matrix", EncodeValue(node.localMatrix) }, { "children", std::move(children) } };
}

/// @brief 階層の深さを制限してNodeを復元
/// @param json Nodeの保存表現
/// @param depth 現在の階層の深さ
/// @return 復元したNode
ModelNode DecodeNode(const Json& json, std::size_t depth = 0) {
    if (depth > 256) {
        throw std::runtime_error("Modelキャッシュの階層が深すぎます");
    }
    ModelNode node;
    node.name = json.at("name").get<std::string>();
    node.transform = DecodeValue<QuaternionTransform>(json.at("transform"));
    node.localMatrix = DecodeValue<Matrix4x4>(json.at("matrix"));
    for (const auto& child : json.at("children")) {
        node.children.push_back(DecodeNode(child, depth + 1));
    }
    return node;
}

/// @brief Animationの変換済みTrackと再生設定を保存
/// @param animations 保存するAnimation集合
/// @return Animation集合の保存表現
Json EncodeAnimations(const AnimationSet& animations) {
    Json clips = Json::array();
    std::vector<std::string> names;
    for (const auto& [name, clip] : animations.GetClips()) {
        (void)clip;
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        const auto& clip = *animations.FindClip(name);
        Json nodes = Json::array();
        for (const auto& [nodeName, node] : clip.nodeAnimations) {
            nodes.push_back({ { "name", nodeName },
                { "translate", EncodeArray(node.translate.keyframes.data(), node.translate.keyframes.size()) },
                { "rotate", EncodeArray(node.rotate.keyframes.data(), node.rotate.keyframes.size()) },
                { "scale", EncodeArray(node.scale.keyframes.data(), node.scale.keyframes.size()) } });
        }
        clips.push_back({ { "name", name }, { "duration", clip.duration }, { "loop", clip.loop },
            { "speed", clip.playbackSpeed }, { "blend", clip.blendDuration }, { "nodes", std::move(nodes) } });
    }
    return { { "default", animations.GetDefaultClipName() }, { "clips", std::move(clips) } };
}

/// @brief Trackと標準Clipを復元
/// @param json Animation集合の保存表現
/// @return 復元したAnimation集合
AnimationSet DecodeAnimations(const Json& json) {
    AnimationSet animations;
    for (const auto& value : json.at("clips")) {
        AnimationClip clip;
        clip.duration = value.at("duration").get<float>();
        clip.loop = value.at("loop").get<bool>();
        clip.playbackSpeed = value.at("speed").get<float>();
        clip.blendDuration = value.at("blend").get<float>();
        if (!std::isfinite(clip.duration) || clip.duration < 0.0f ||
            !std::isfinite(clip.playbackSpeed) || clip.playbackSpeed < 0.0f ||
            !std::isfinite(clip.blendDuration) || clip.blendDuration < 0.0f) {
            throw std::runtime_error("Animation設定が不正");
        }
        for (const auto& nodeJson : value.at("nodes")) {
            NodeAnimation node;
            node.translate.keyframes = DecodeArray<KeyframeVector3>(nodeJson.at("translate"));
            node.rotate.keyframes = DecodeArray<KeyframeQuaternion>(nodeJson.at("rotate"));
            node.scale.keyframes = DecodeArray<KeyframeVector3>(nodeJson.at("scale"));
            clip.nodeAnimations.emplace(nodeJson.at("name").get<std::string>(), std::move(node));
        }
        if (!animations.AddClip(value.at("name").get<std::string>(), std::move(clip))) {
            throw std::runtime_error("AnimationClipが不正");
        }
    }
    const auto defaultName = json.at("default").get<std::string>();
    if (!defaultName.empty() && !animations.SetDefaultClip(defaultName)) {
        throw std::runtime_error("標準AnimationClipが不正");
    }
    return animations;
}

/// @brief 配置先に依存しないModelデータを生成
/// @param sourcePath 元Modelのパス
/// @param data 保存するModelデータ
/// @return Modelデータの保存表現
Json EncodeAsset(const fs::path& sourcePath, const MadoEngine::ModelAssetData& data) {
    const auto& model = data.modelData;
    Json materials = Json::array();
    for (const auto& path : model.materialPaths) {

        // 開発PCの絶対パスを保存せず、配布先のModel位置を基準にTexture参照を復元
        const auto relative = fs::absolute(path).lexically_relative(fs::absolute(sourcePath.parent_path()));
        if (relative.empty()) {

            // 元Asset内の複合ドライブ表記などもTexture名の解決に必要なため欠落を回避
            materials.push_back({ { "path", path }, { "relative", false } });
        } else {
            materials.push_back({ { "path", EncodePath(relative) }, { "relative", true } });
        }
    }
    Json skin = Json::array();
    for (const auto& [name, joint] : model.skinClusterData) {
        skin.push_back({ { "name", name }, { "inverseBind", EncodeValue(joint.inverseBindPoseMatrix) },
            { "weights", EncodeArray(joint.vertexWeights.data(), joint.vertexWeights.size()) } });
    }
    return { { "vertices", EncodeArray(model.vertices.data(), model.vertices.size()) },
        { "indices", EncodeArray(model.indices.data(), model.indices.size()) },
        { "subMeshes", EncodeArray(model.subMeshes.data(), model.subMeshes.size()) },
        { "materials", std::move(materials) },
        { "colors", EncodeArray(model.materialColors.data(), model.materialColors.size()) },
        { "boundsMin", EncodeValue(model.bounds.min) }, { "boundsMax", EncodeValue(model.bounds.max) },
        { "boundsValid", model.bounds.isValid }, { "root", EncodeNode(model.rootNode) },
        { "skin", std::move(skin) }, { "animations", EncodeAnimations(data.animationSet) } };
}

/// @brief 描画範囲を検証してModelデータを復元
/// @param sourcePath 元Modelのパス
/// @param json Modelデータの保存表現
/// @return 復元したModelデータ
MadoEngine::ModelAssetData DecodeAsset(const fs::path& sourcePath, const Json& json) {
    MadoEngine::ModelAssetData data;
    auto& model = data.modelData;
    model.vertices = DecodeArray<ModelVertexData>(json.at("vertices"));
    model.indices = DecodeArray<std::uint32_t>(json.at("indices"));
    model.subMeshes = DecodeArray<ModelSubMesh>(json.at("subMeshes"));
    model.materialColors = DecodeArray<Vector4>(json.at("colors"));
    for (const auto& path : json.at("materials")) {
        const auto value = path.at("path").get<std::string>();
        if (path.at("relative").get<bool>()) {
            model.materialPaths.push_back((sourcePath.parent_path() / DecodePath(value)).lexically_normal().generic_string());
        } else {
            model.materialPaths.push_back(value);
        }
    }
    if (model.vertices.empty() || model.indices.empty() ||
        model.materialPaths.empty() || model.materialColors.size() != model.materialPaths.size()) {
        throw std::runtime_error("ModelキャッシュのMeshが不正");
    }
    for (const auto index : model.indices) {
        if (index >= model.vertices.size()) {
            throw std::runtime_error("Modelキャッシュの頂点参照が範囲外");
        }
    }
    for (const auto& mesh : model.subMeshes) {
        if (mesh.materialIndex >= model.materialPaths.size() || mesh.indexStart > model.indices.size() ||
            mesh.indexCount > model.indices.size() - mesh.indexStart) {
            throw std::runtime_error("Modelキャッシュの描画範囲が不正");
        }
    }
    model.bounds.min = DecodeValue<Vector3>(json.at("boundsMin"));
    model.bounds.max = DecodeValue<Vector3>(json.at("boundsMax"));
    model.bounds.isValid = json.at("boundsValid").get<bool>();
    model.rootNode = DecodeNode(json.at("root"));
    for (const auto& jointJson : json.at("skin")) {
        JointWeightData joint;
        joint.inverseBindPoseMatrix = DecodeValue<Matrix4x4>(jointJson.at("inverseBind"));
        joint.vertexWeights = DecodeArray<VertexWeightData>(jointJson.at("weights"));
        for (const auto& weight : joint.vertexWeights) {
            if (weight.vertexIndex >= model.vertices.size() || !std::isfinite(weight.weight)) {
                throw std::runtime_error("ModelキャッシュのWeightが不正");
            }
        }
        model.skinClusterData.emplace(jointJson.at("name").get<std::string>(), std::move(joint));
    }
    data.animationSet = DecodeAnimations(json.at("animations"));
    return data;
}

/// @brief 元ファイルの状態を相対パスと内容識別値で記録
/// @param sourcePath 元Modelのパス
/// @param dependencies 依存ファイル集合
/// @return 依存関係の保存表現
Json EncodeDependencies(const fs::path& sourcePath, const std::vector<fs::path>& dependencies) {
    std::set<fs::path> paths;
    paths.insert(fs::absolute(sourcePath).lexically_normal());
    for (const auto& path : dependencies) {
        paths.insert(fs::absolute(path).lexically_normal());
    }
    Json entries = Json::array();
    for (const auto& path : paths) {
        const bool exists = fs::is_regular_file(path);
        Json entry = { { "path", EncodePath(path.lexically_relative(fs::absolute(sourcePath.parent_path()))) }, { "exists", exists } };
        if (exists) {
            std::uint64_t fingerprint = 0;
            if (!MadoEngine::ModelCache::ComputeFingerprint(path, fingerprint)) {
                throw std::runtime_error("Model依存ファイルを読み込めません");
            }
            entry["size"] = fs::file_size(path);
            entry["time"] = fs::last_write_time(path).time_since_epoch().count();
            entry["hash"] = fingerprint;
        }
        entries.push_back(std::move(entry));
    }
    return entries;
}

/// @brief 依存ファイルの存在状態と更新内容を検証
/// @param sourcePath 元Modelのパス
/// @param entries 依存関係の保存表現
/// @return すべての依存関係が一致する場合はtrue
bool DependenciesMatch(const fs::path& sourcePath, const Json& entries) {
    if (!entries.is_array() || entries.empty()) {
        return false;
    }
    for (const auto& entry : entries) {
        const auto path = sourcePath.parent_path() / DecodePath(entry.at("path").get<std::string>());
        const bool exists = fs::is_regular_file(path);
        if (exists != entry.at("exists").get<bool>()) {
            return false;
        }
        if (!exists) {
            continue;
        }
        if (fs::file_size(path) != entry.at("size").get<std::uint64_t>()) {
            return false;
        }
        if (fs::last_write_time(path).time_since_epoch().count() != entry.at("time").get<std::int64_t>()) {

            // ZIP展開で更新時刻の精度が変わっても、内容が一致すれば配布済みキャッシュを利用
            std::uint64_t fingerprint = 0;
            if (!MadoEngine::ModelCache::ComputeFingerprint(path, fingerprint) || fingerprint != entry.at("hash").get<std::uint64_t>()) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

namespace MadoEngine {

ModelCache::ModelCache(std::filesystem::path modelRoot) : modelRoot_(std::move(modelRoot)) {}

std::filesystem::path ModelCache::GetPath(const std::filesystem::path& sourcePath) const {
    const auto relative = fs::absolute(sourcePath).lexically_normal().lexically_relative(fs::absolute(modelRoot_).lexically_normal());
    if (relative.empty() || *relative.begin() == "..") {
        return {};
    }
    auto path = modelRoot_ / "Cache" / relative;
    path += ".modelcache";
    return path;
}

bool ModelCache::Load(const std::filesystem::path& sourcePath, ModelAssetData& outData, std::uint64_t* converterSignature) const {
    try {
        const auto path = GetPath(sourcePath);
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input) {
            return false;
        }
        const auto fileSize = input.tellg();
        input.seekg(0);
        std::array<char, 8> magic{};
        std::uint32_t version = 0;
        std::uint64_t size = 0;
        std::uint64_t expectedHash = 0;
        input.read(magic.data(), magic.size());
        input.read(reinterpret_cast<char*>(&version), sizeof(version));
        input.read(reinterpret_cast<char*>(&size), sizeof(size));
        input.read(reinterpret_cast<char*>(&expectedHash), sizeof(expectedHash));
        if (!input || magic != kMagic || version != kVersion || size > kMaximumPayload ||
            fileSize < 0 || static_cast<std::uint64_t>(fileSize) != kHeaderSize + size) {
            return false;
        }

        // 破損データを可変長デコーダーへ渡す前にサイズと全体識別値を検証
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        std::uint64_t hash = kHashBasis;
        HashBytes(hash, bytes.data(), bytes.size());
        if (!input || hash != expectedHash) {
            return false;
        }
        const Json json = Json::from_cbor(bytes);
        if (!DependenciesMatch(sourcePath, json.at("dependencies"))) {
            return false;
        }
        auto data = DecodeAsset(sourcePath, json.at("asset"));
        if (converterSignature) {
            *converterSignature = json.at("converter").get<std::uint64_t>();
        }
        outData = std::move(data);
        return true;
    } catch (const std::exception&) {

        // 欠損、旧形式、破損、依存変更はいずれも元Modelの読み込み経路へ復旧
        return false;
    }
}

bool ModelCache::Save(const std::filesystem::path& sourcePath, const ModelAssetData& data,
    const std::vector<std::filesystem::path>& dependencies, std::uint64_t converterSignature) const {
    fs::path temporaryPath;
    try {
        const auto path = GetPath(sourcePath);
        if (path.empty()) {
            return false;
        }
        const Json json = { { "converter", converterSignature }, { "dependencies", EncodeDependencies(sourcePath, dependencies) },
            { "asset", EncodeAsset(sourcePath, data) } };
        const auto bytes = Json::to_cbor(json);
        const std::uint64_t size = bytes.size();
        if (size > kMaximumPayload) {
            return false;
        }
        std::uint64_t hash = kHashBasis;
        HashBytes(hash, bytes.data(), bytes.size());
        fs::create_directories(path.parent_path());
        temporaryPath = path;
        temporaryPath += L".tmp." + std::to_wstring(GetCurrentProcessId());
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        output.write(kMagic.data(), kMagic.size());
        output.write(reinterpret_cast<const char*>(&kVersion), sizeof(kVersion));
        output.write(reinterpret_cast<const char*>(&size), sizeof(size));
        output.write(reinterpret_cast<const char*>(&hash), sizeof(hash));
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        output.close();

        // 実行中に不完全なキャッシュを参照しないよう、完成したファイルだけを公開
        if (output && MoveFileExW(temporaryPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            return true;
        }
    } catch (const std::exception&) {
    }
    if (!temporaryPath.empty()) {
        std::error_code error;
        fs::remove(temporaryPath, error);
    }
    return false;
}

bool ModelCache::ComputeFingerprint(const std::filesystem::path& path, std::uint64_t& outFingerprint) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    std::array<std::uint8_t, 65536> bytes{};
    std::uint64_t hash = kHashBasis;
    while (input) {
        input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        HashBytes(hash, bytes.data(), static_cast<std::size_t>(input.gcount()));
    }
    if (!input.eof()) {
        return false;
    }
    outFingerprint = hash;
    return true;
}

}
