#include "ModelData.h"

ModelData LoadObject3dFile(const std::string& filepath) {
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