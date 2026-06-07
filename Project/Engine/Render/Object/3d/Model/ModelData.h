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
	ModelNode rootNode;                                     // ノード階層のルート
};

ModelData LoadObject3dFile(const std::string& filepath);