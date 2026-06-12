#pragma once
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Math/Function/MatrixFunction.h"
#include "Math/Transform.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include<cassert>
#include <fstream>
#include <sstream>
#include <filesystem>

struct ModelVertexData {
	Vector4 position; // 位置
	Vector2 texcoord; // UV座標
	Vector3 normal;   // 法線
    float pad[2];
    float pad2;
};

struct ModelMaterial {
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};             // 色
    int32_t enableLighting = true;                        // ライティングの有効化
    int32_t useEnvironmentMap = 0;                        // 環境マップの使用
    float pad1[2];
    Matrix4x4 uvTransformMatrix = Matrix::MakeIdentity(); // UV変換行列
    float shininess = 10000.0f;                           // 光沢度
    float pad2[3];
};

struct ModelTransformationMatrix {
	Matrix4x4 WVP = Matrix::MakeIdentity();   // ワールドビュー射影行列
	Matrix4x4 World = Matrix::MakeIdentity(); // ワールド行列
	Matrix4x4 WorldInverseTranspose;          // ワールド行列の逆行列の転置（法線変換用）
};

struct ModelSubMesh {
	uint32_t indexStart;    // インデックスバッファ内の開始位置
	uint32_t indexCount;    // インデックスの数
	uint32_t materialIndex; // マテリアルインデックス（マルチマテリアル対応）
};

/// @brief モデルのローカル空間AABB
struct ModelBounds {
    Vector3 min = {};
    Vector3 max = {};
    bool isValid = false;
};

struct ModelNode {
    QuaternionTransform transform;
    Matrix4x4 localMatrix;
    std::string name;
    std::vector<ModelNode> children;

    ModelNode ReadNode(aiNode* node) {
        ModelNode result;
        aiVector3D scale, translate;
        aiQuaternion rotate;
        node->mTransformation.Decompose(scale, rotate, translate);               // assimpの行列からSRTを抽出する関数を利用
        result.transform.scale = { scale.x, scale.y, scale.z };                  // Scaleはそのまま
        result.transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w };  // x軸を反転、さらに回転方向が逆なので軸を反転させる
        result.transform.translate = { -translate.x, translate.y, translate.z }; // x軸を反転
        result.localMatrix = Matrix::MakeAffineAnimation(result.transform.scale, result.transform.rotate, result.transform.translate);
        result.name = node->mName.C_Str(); // Node名を格納
        result.children.resize(node->mNumChildren); // 子供の数だけ確保
        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
            // 再帰的に読んで階層構造を作っていく
            result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
        }
        return result;
    }
};

struct VertexWeightData {
	float weight;         // ウェイト値
	uint32_t vertexIndex; // 頂点インデックス
};

struct JointWeightData {
    Matrix4x4 inverseBindPoseMatrix;             // バインドポーズの逆行列
    std::vector<VertexWeightData> vertexWeights; // 頂点ウェイトデータ
};

struct ModelData {
	std::map<std::string, JointWeightData> skinClusterData; // スキンクラスター（ジョイント）ごとの頂点ウェイトデータ
	std::vector<ModelVertexData> vertices;                  // 全サブメッシュの頂点を連結して格納
	std::vector<uint32_t> indeces;                          // 全サブメッシュのインデックスを連結して格納
    std::vector<std::string> materialPaths;                 // マルチマテリアル対応
    std::vector<ModelSubMesh> subMeshes;                    // サブメッシュ情報
	ModelBounds bounds;                                     // ローカル空間AABB
	ModelNode rootNode;                                     // ノード階層のルート
};

inline ModelData LoadObject3dFile(const std::string& filepath) {
    ModelData modelData;
    std::filesystem::path path(filepath);
    std::string directoryPath = path.parent_path().string();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath.c_str(),
        aiProcess_FlipWindingOrder |
        aiProcess_FlipUVs |
        aiProcess_Triangulate);

    assert(scene && scene->HasMeshes());

    modelData.rootNode = modelData.rootNode.ReadNode(scene->mRootNode);

    // マテリアルを先に読み込む
    modelData.materialPaths.resize(scene->mNumMaterials);
    for (uint32_t matIndex = 0; matIndex < scene->mNumMaterials; ++matIndex) {
        aiMaterial* material = scene->mMaterials[matIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                std::string textureFilePath = directoryPath + "/" + texPath.C_Str();

                // uvChecker.pngが設定されている場合、white1x1.pngに置き換え
                if (textureFilePath.find("uvChecker.png") != std::string::npos) {
                    modelData.materialPaths[matIndex] = "Assets/.EngineResource/white1x1.png";
                } else {
                    modelData.materialPaths[matIndex] = textureFilePath;
                }
            } else {
                modelData.materialPaths[matIndex] = "Assets/.EngineResource/white1x1.png";
            }
        } else {
            modelData.materialPaths[matIndex] = "Assets/.EngineResource/white1x1.png";
        }
    }

    // マテリアルがない場合のデフォルト
    if (modelData.materialPaths.empty()) {
        modelData.materialPaths.push_back("Assets/.EngineResource/white1x1.png");
    }

    // 各メッシュを処理
    uint32_t indexOffset = 0;
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        assert(mesh->HasNormals());
        // テクスチャ座標のチェックを削除
        // assert(mesh->HasTextureCoords(0));

        uint32_t vertexStart = static_cast<uint32_t>(modelData.vertices.size());

        // 頂点データを追加
        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            ModelVertexData vertex;
            aiVector3D& position = mesh->mVertices[vertexIndex];
            aiVector3D& normal = mesh->mNormals[vertexIndex];

            // 右手系→左手系への変換
            vertex.position = { -position.x, position.y, position.z, 1.0f };
            vertex.normal = { -normal.x, normal.y, normal.z };

            const Vector3 localPosition = {
                vertex.position.x,
                vertex.position.y,
                vertex.position.z
            };
            if (!modelData.bounds.isValid) {
                modelData.bounds.min = localPosition;
                modelData.bounds.max = localPosition;
                modelData.bounds.isValid = true;
            } else {
                modelData.bounds.min.x = (std::min)(modelData.bounds.min.x, localPosition.x);
                modelData.bounds.min.y = (std::min)(modelData.bounds.min.y, localPosition.y);
                modelData.bounds.min.z = (std::min)(modelData.bounds.min.z, localPosition.z);
                modelData.bounds.max.x = (std::max)(modelData.bounds.max.x, localPosition.x);
                modelData.bounds.max.y = (std::max)(modelData.bounds.max.y, localPosition.y);
                modelData.bounds.max.z = (std::max)(modelData.bounds.max.z, localPosition.z);
            }

            // テクスチャ座標が存在する場合のみ取得、なければデフォルト値(0,0)
            if (mesh->HasTextureCoords(0)) {
                aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
                vertex.texcoord = { texcoord.x, texcoord.y };
            } else {
                vertex.texcoord = { 0.0f, 0.0f };
            }

            modelData.vertices.push_back(vertex);
        }

        // インデックスデータを追加
        uint32_t indexStart = static_cast<uint32_t>(modelData.indeces.size());
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            aiFace& face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3); // 三角形のみ対応

            for (uint32_t element = 0; element < face.mNumIndices; ++element) {
                uint32_t vertexIndex = face.mIndices[element];
                modelData.indeces.push_back(vertexStart + vertexIndex);
            }
        }
        uint32_t indexCount = static_cast<uint32_t>(modelData.indeces.size()) - indexStart;

        // サブメッシュ情報を追加
        ModelSubMesh subMesh;
        subMesh.indexStart = indexStart;
        subMesh.indexCount = indexCount;
        subMesh.materialIndex = mesh->mMaterialIndex;
        // マテリアルインデックスが範囲外の場合はクランプ
        if (subMesh.materialIndex >= modelData.materialPaths.size()) {
            subMesh.materialIndex = 0;
        }
        modelData.subMeshes.push_back(subMesh);

        // スキニング情報の処理
        for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            aiBone* bone = mesh->mBones[boneIndex];
            std::string jointName = bone->mName.C_Str();
            JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

            aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
            aiVector3D scale, translate;
            aiQuaternion rotate;
            bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
            Matrix4x4 bindPoseMatrix = Matrix::MakeAffineAnimation(
                { scale.x, scale.y, scale.z }, { rotate.x, -rotate.y, -rotate.z, rotate.w }, { -translate.x, translate.y, translate.z });
            jointWeightData.inverseBindPoseMatrix = Matrix::Inverse(bindPoseMatrix);

            for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight, vertexStart + bone->mWeights[weightIndex].mVertexId });
            }
        }
    }

    return modelData;
}
