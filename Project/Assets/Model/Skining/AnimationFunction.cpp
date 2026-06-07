#include "AnimationFunction.h"
#include "Math/MatrixFunction/MatrixFunction.h"
#include "Math/Quaternion/QuaternionFunction.h"
#include "Utility/Easing/Easing.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Animation LoadAnimationFile(const std::string& filename, int index)
{
	Animation animation; // 今回作るアニメーション

	Assimp::Importer importer;
	std::string filePath = "resources/model/" + filename;

	// モデルと同じでOK。資料では 0 になってるが、既に使ってるフラグがあればそれでいい
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene && "Failed to load animation file");
	assert(scene->mNumAnimations != 0 && "No animations in file");

	// 指定されたアニメーションを指定
	index = std::clamp(index, 0, int(scene->mNumAnimations));
	aiAnimation* animationAssimp = scene->mAnimations[index];

	// 時間の単位を秒に変換
	const double ticksPerSecond =
		(animationAssimp->mTicksPerSecond != 0.0)
		? animationAssimp->mTicksPerSecond
		: 1.0; // 0防止

	animation.duration = static_cast<float>(
		animationAssimp->mDuration / ticksPerSecond);

	// 各ノードの Animation(channel) を解析
	for (uint32_t channelIndex = 0;
		channelIndex < animationAssimp->mNumChannels;
		++channelIndex)
	{
		aiNodeAnim* nodeAnimationAssimp =
			animationAssimp->mChannels[channelIndex];

		// ノード名で NodeAnimation を取得
		NodeAnimation& nodeAnimation =
			animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

		// --------- Translate ----------
		for (uint32_t keyIndex = 0;
			keyIndex < nodeAnimationAssimp->mNumPositionKeys;
			++keyIndex)
		{
			aiVectorKey& keyAssimp =
				nodeAnimationAssimp->mPositionKeys[keyIndex];

			KeyframeVector3 keyframe;
			keyframe.time = static_cast<float>(
				keyAssimp.mTime / ticksPerSecond);  // 秒に変換

			// 右手→左手変換（資料どおり：x反転だけ or x,z反転など、
			// 自分のモデル読み込みと揃えること）
			keyframe.value = {
				-keyAssimp.mValue.x,
				 keyAssimp.mValue.y,
				 keyAssimp.mValue.z
			};

			nodeAnimation.translate.keyframes.push_back(keyframe);
		}

		// --------- Rotate ----------
		for (uint32_t keyIndex = 0;
			keyIndex < nodeAnimationAssimp->mNumRotationKeys;
			++keyIndex)
		{
			aiQuatKey& keyAssimp =
				nodeAnimationAssimp->mRotationKeys[keyIndex];

			KeyframeQuaternion keyframe;
			keyframe.time = static_cast<float>(
				keyAssimp.mTime / ticksPerSecond);

			// Assimp は右手系。自分のエンジンに合わせて変換
			const aiQuaternion& q = keyAssimp.mValue;

			// 例：Y,Z 反転で右手→左手 にするパターン
			Quaternion rotate{};
			rotate.x = q.x;
			rotate.y = -q.y;
			rotate.z = -q.z;
			rotate.w = q.w;

			keyframe.value = rotate;
			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}

		// --------- Scale ----------
		for (uint32_t keyIndex = 0;
			keyIndex < nodeAnimationAssimp->mNumScalingKeys;
			++keyIndex)
		{
			aiVectorKey& keyAssimp =
				nodeAnimationAssimp->mScalingKeys[keyIndex];

			KeyframeVector3 keyframe;
			keyframe.time = static_cast<float>(
				keyAssimp.mTime / ticksPerSecond);

			// Scale はそのままでOK
			keyframe.value = {
				keyAssimp.mValue.x,
				keyAssimp.mValue.y,
				keyAssimp.mValue.z
			};

			nodeAnimation.scale.keyframes.push_back(keyframe);
		}
	}

	// 解析完了
	return animation;
}

Skeleton CreateSkeleton(const ModelNode& rootNode) {
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	// 名前とindexのマッピングを行いアクセスしやすくする
	for (const Joint& joint : skeleton.joints) {
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	return skeleton;
}

SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Skeleton& skeleton, const ModelData& modelData, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t srvIndex)
{
	SkinCluster skinCluster;

	// ★デバッグログ追加
	char buffer[512];
	sprintf_s(buffer, "=== CreateSkinCluster Debug ===\n");
	OutputDebugStringA(buffer);

	sprintf_s(buffer, "Joint count: %zu\n", skeleton.joints.size());
	OutputDebugStringA(buffer);

	sprintf_s(buffer, "Vertex count: %zu\n", modelData.vertices.size());
	OutputDebugStringA(buffer);

	// palette用のResourceを確保
	size_t paletteSize = sizeof(WellForGPU) * skeleton.joints.size();
	sprintf_s(buffer, "Palette buffer size: %zu bytes (WellForGPU size: %zu)\n",
		paletteSize, sizeof(WellForGPU));
	OutputDebugStringA(buffer);

	OutputDebugStringA("Creating palette resource...\n");
	skinCluster.paletteResource = CreateBufferResource(device.Get(), paletteSize);
	OutputDebugStringA("Palette resource created successfully!\n");

	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette, skeleton.joints.size() }; // spanを使ってアクセスするようにする
	skinCluster.paletteSrvHandle.first = GetCPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, srvIndex);
	skinCluster.paletteSrvHandle.second = GetGPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, srvIndex);

	// palette用のsrvを作成。StructuredBufferでアクセスできるようにする。
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = UINT(skeleton.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
	device->CreateShaderResourceView(skinCluster.paletteResource.Get(), &paletteSrvDesc, skinCluster.paletteSrvHandle.first);

	OutputDebugStringA("Palette SRV created successfully!\n");

	// influence用のResourceを確保。頂点ごとにinfluence情報を追加できるようにする
	size_t influenceSize = sizeof(VertexInfluence) * modelData.vertices.size();
	sprintf_s(buffer, "Influence buffer size: %zu bytes (VertexInfluence size: %zu)\n",
		influenceSize, sizeof(VertexInfluence));
	OutputDebugStringA(buffer);

	OutputDebugStringA("Creating influence resource...\n");
	skinCluster.influenceResource = CreateBufferResource(device.Get(), influenceSize);
	OutputDebugStringA("Influence resource created successfully!\n");

	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size());  // 0埋め。weightを0にしておく。
	skinCluster.mappedInfluence = { mappedInfluence, modelData.vertices.size() };

	// Influence用のVBVを作成
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// InverseBindPoseMatrixを格納する場所を作成して、単位行列で埋める
	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), MakeIdentityMatrix);

	for (const auto& jointWeight : modelData.skinClusterData) { // ModelのSkinClusterの情報を解析
		auto it = skeleton.jointMap.find(jointWeight.first);    // jointWeight.firstはjoint名なので、skeletonに対象となるjointが含まれているか判断
		if (it == skeleton.jointMap.end()) {                    // そんな名前のJointは存在しない。なので次に回す
			continue;
		}

		// (*it).secondにはjointのindexが入っているので、該当のindexのinverseBindPoseMatrixを代入
		skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;

		for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
			auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex]; // 該当のvertexIndexのinfluence情報を参照しておく
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {                   // 空いているところに入れる
				if (currentInfluence.weights[index] == 0.0f) {                              // weight==0が空いている状態なので、その場所にweightとjointのindexを代入
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}

	OutputDebugStringA("CreateSkinCluster completed successfully!\n");
	OutputDebugStringA("===============================\n");

	return skinCluster;
}

int32_t CreateJoint(const ModelNode& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {

	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentityMatrix();
	joint.transform = node.transform;
	joint.index = int32_t(joints.size()); // 現在登録されている数をIndexに
	joint.parent = parent;
	joints.push_back(joint); // SkeletonのJoint列に追加

	// 子Jontを作成し、そのIndexを登録
	for (const ModelNode& child : node.children) {
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	// 自身のIndexを返す
	return joint.index;
}

Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time) {

	assert(!keyframes.empty());   // キーがないと値がわからないのでダメ

	// キーが1つだけ、または time が最初のキー以前なら最初の値
	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	// 範囲内を探す
	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;

		if (keyframes[index].time <= time &&
			time <= keyframes[nextIndex].time)
		{
			float t =
				(time - keyframes[index].time) /
				(keyframes[nextIndex].time - keyframes[index].time);

			return MyEasing::Lerp(keyframes[index].value,
				keyframes[nextIndex].value,
				t);
		}
	}

	// ここまで来たら最後のキーより後なので一番最後の値
	return (*keyframes.rbegin()).value;
}

Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time) {

	assert(!keyframes.empty());

	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;

		if (keyframes[index].time <= time &&
			time <= keyframes[nextIndex].time)
		{
			float t =
				(time - keyframes[index].time) /
				(keyframes[nextIndex].time - keyframes[index].time);

			return Slerp(keyframes[index].value,
				keyframes[nextIndex].value,
				t);
		}
	}

	return (*keyframes.rbegin()).value;
}

void UpdateAnimation(Skeleton& skeleton) {
	// すべてのJointを更新。親が若いので通常ループで処理可能になっている
	for (Joint& joint : skeleton.joints) {
		joint.localMatrix = MakeAffineAnimationMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) { // 親がいれば親の行列を掛ける
			joint.skeletonSpaceMatrix = joint.localMatrix * skeleton.joints[*joint.parent].skeletonSpaceMatrix;
		} else { // 親がいないのでlocalMatrixとskeletonSpaceMatrixは一致する
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

void UpdateCluster(SkinCluster& skinCluster, const Skeleton& skeleton) {
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix =
			skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			TransposeMatrix(InverseMatrix(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}

void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime) {
	for (Joint& joint : skeleton.joints) {
		// 対象のJointのAnimationがあれば、値の適用を行う。下記のif文はC++17から可能になった初期化付きif文。
		if (auto it = animation.nodeAnimations.find(joint.name); it != animation.nodeAnimations.end()) {
			const NodeAnimation& rootNodeAnimation = (*it).second;
			joint.transform.translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
			joint.transform.rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
			joint.transform.scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);
		}
	}
}