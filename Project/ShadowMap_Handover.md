# ShadowMap 引継ぎメモ

## 目的

DirectionalLight 用の ShadowMap 実装を進めている。

最終的にやりたいことは以下。

- 通常3D描画前にシャドウマップ用Depthパスを実行する
- Model単位で `castShadow` と `receiveShadow` を制御する
- Static / Instanced / Skinning / Animated のModelが影を落とせるようにする
- 通常描画時にShadowMapを参照して、DirectionalLightのdiffuse/specularだけを弱める
- ambientは残す

## 現在の状態

ShadowMapの基本実装は入っている。

- `Engine/Render/Shadow/ShadowMap.h`
- `Engine/Render/Shadow/ShadowMap.cpp`

主な仕様。

- ShadowMapサイズは `2048 x 2048`
- DepthStencilBufferを内部に保持
- `Begin()` でDSV設定、深度クリア
- `End()` でSRV参照可能状態へ遷移
- `GetSRVGPUHandle()` あり
- Orthographic範囲は現在 `200 x 200`
- near/farは `0.1 / 1000.0`
- `DepthBias = 1000`
- `SlopeScaledDepthBias = 1.5`
- `CullMode::None`

## 描画順

`Engine/.Execution/Execution.cpp` の `PreDraw()` 内で、通常3D描画前に `RenderShadowMap()` を呼んでいる。

流れは以下。

1. ImGuiフレーム開始
2. CommandList開始
3. SRV DescriptorHeap設定
4. `RenderShadowMap(...)`
5. 通常シーンカラーRTへ描画開始

`RenderShadowMap()` の中では以下を行う。

1. `LightManager` から現在シーンのDirectionalLightを取得
2. `ShadowMap::UpdateLightViewProjection(...)`
3. `ShadowMap::Begin(...)`
4. `ModelManager::DrawShadowMap(...)`
5. `ShadowMap::End(...)`
6. `ModelManager::SetShadowMap(...)`

## Light ViewProjection

`ShadowMap::UpdateLightViewProjection()` でDirectionalLightの方向からライト視点行列を作っている。

現在の式。

```cpp
Vector3 lightDirection = NormalizeShadowLightDirection(directionalLight.direction);
Vector3 lightPosition = focusPosition - lightDirection * ((kShadowNearClip + kShadowFarClip) * 0.5f);

lightViewMatrix_ = MakeLookAtLH(lightPosition, focusPosition, { 0.0f, 1.0f, 0.0f });
lightProjectionMatrix_ = Matrix::MakeOrthographic(
    -kShadowOrthoWidth * 0.5f,
    kShadowOrthoHeight * 0.5f,
    kShadowOrthoWidth * 0.5f,
    -kShadowOrthoHeight * 0.5f,
    kShadowNearClip,
    kShadowFarClip
);
lightViewProjectionMatrix_ = Matrix::Multiply(lightViewMatrix_, lightProjectionMatrix_);
```

TestSceneでは `shadowFocusPosition` はPlayer位置を返している。

- `Application/.SceneManager/Scene/Test.cpp`
- `Test::GetShadowFocusPosition()`

そのため、Player自身のLight NDCは常にほぼ中心になる。

実測表示。

- `Player Light NDC: (-0.000, 0.000, 0.500)`

これはPlayerをShadowCameraの中心に置いているため正常。

## RootSignature / PSO

ShadowMap用RootSignatureは `Engine/Shader/RootSignatureManager.cpp` にある。

- `ShadowMap.RootSig`
- b0: Shadow用WVP
- VSのみ

Static/Animated用Shadow PSOは `ShadowMap::CreatePSODesc()`。

- `vsKey = "Object3d/Shadow/ShadowMap.VS"`
- `psKey.clear()`
- `rootSigKey = "ShadowMap.RootSig"`
- RTVなし
- DSVのみ

Skinning用Shadow PSOは `Engine/Render/Object/3d/Model/Model.cpp` の `CreateSkinningShadowPSODesc()`。

- `inputLayout = SkiningModel`
- `vsKey = "Object3d/Shadow/SkinningShadowMap.VS"`
- `rootSigKey = "SkinningModel.RootSig"`

Instanced用Shadow PSOは `Engine/Render/Object/3d/Model/InstancedModel.cpp` 側。

- `vsKey = "Object3d/Shadow/InstancedShadowMap.VS"`

## Shadow用Shader

Static/Animated用。

- `Assets/Shader/Object3d/Shadow/ShadowMap.VS.hlsl`

Skinning用。

- `Assets/Shader/Object3d/Shadow/SkinningShadowMap.VS.hlsl`

Skinning Shadow VSは以下を使う。

- `StructuredBuffer<Well> gMatrixPalette : register(t1);`
- `ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);`

C++側の `Model::DrawShadow()` でもSkinning時に以下をセットしている。

- root parameter 1: `transformationResource_`
- root parameter 4: `skinClusterData_.paletteSrvHandle.second`

## Model側のShadow対応

`Engine/Render/Object/3d/Model/Model.h`

- `SetCastShadow(bool)`
- `CanCastShadow()`
- `SetReceiveShadow(bool)`
- `CanReceiveShadow()`
- `DrawShadow(const Matrix4x4& lightViewProjection)`
- `SetShadowMap(...)`

`Model::DrawShadow()` は以下。

1. `sharedData_ / isVisible_ / castShadow_` を確認
2. `UpdateShadowTransformGpuData(lightViewProjection)`
3. ModelTypeに応じてShadow PSOを選択
4. RootSignature / PSO設定
5. VB/IB設定
6. Skinningの場合はTransform CBVとPalette SRVを設定
7. DrawIndexedInstanced

`UpdateShadowTransformGpuData()` では、通常描画と同じTransformationBufferにShadow用WVPを書いている。

## ModelManager側のShadow対応

`Engine/Render/Object/3d/Model/ModelManager.cpp`

- `DrawShadowMap(...)`
- `DrawShadowMapLayer(...)`
- `DrawShadowMapLayerMask(...)`
- `SetShadowMap(...)`
- `SetShadowMapLayerMask(...)`

`DrawShadowMapLayerMask()` は以下の条件でModelをShadowパスに流す。

- Visible
- SceneType一致
- RenderLayer一致
- CanCastShadow

InstancedModelも同じように処理している。

## 通常描画側の受影

Static/Animated PS。

- `Assets/Shader/Object3d/Model/Model.PS.hlsl`

Skinning PS。

- `Assets/Shader/Object3d/Model/SkinningModel.PS.hlsl`

両方とも `CalculateShadowFactor(float3 worldPosition)` を持つ。

現在の処理。

1. `worldPosition` を `lightViewProjection` でclip変換
2. NDCへ変換
3. ShadowMap UVへ変換
4. 範囲外なら影なし
5. `SampleCmpLevelZero` で3x3 PCF
6. `shadowFactor` でDirectionalLight寄与を弱める
7. ambientは残す

## Debug表示

`Shadow Map Debug` ウィンドウを追加済み。

場所。

- `Engine/.Execution/Execution.cpp`
- `EngineExecution::DrawShadowMapDebugWindow()`

表示内容。

- ShadowMapサイズ
- `shadowFocusPosition`
- Playerモデル描画座標
- Playerモデル座標をLightViewProjection変換したNDC
- X/Y/Zが範囲内か
- ShadowMap SRVプレビュー

現在追加済みのPlayer Shadow Target表示。

- `DrawShadow Called`
- `Found`
- `ModelType`
- `RenderLayer`
- `SceneType`
- `Visible`
- `Scene Match`
- `Layer Match`
- `CastShadow`

## 調査済みの事実

PlayerはShadowMap描画対象に入っている。

実測表示。

- `DrawShadow Called: YES`
- `Found: YES`
- `ModelType: Skinning`
- `RenderLayer: Player`
- `SceneType: 3`
- `Visible: YES`
- `Scene Match: YES`
- `Layer Match: YES`
- `CastShadow: YES`

つまり、Playerの影が出ない原因は `ModelManager` の対象外判定ではない。

Playerは以下で作成されている。

- `Application/Object/Player/Player.cpp`
- `MyModel::Create("Player", "walk", SceneType::Test)`

`walk` は以下。

- `Assets/Model/Skining/walk.gltf`

つまりPlayerはSkinningモデル。

`model_->SetCastShadow(true)` も入っている。

## 現在の症状

Depth Debug SRVを見ると、出ている敵の数に対して影の数が少ない。

特に問題。

- Playerの影が見えない
- PlayerはShadow描画対象に入っている
- PlayerのLight NDCは範囲内
- それでもDepth preview上でPlayerらしき影が見えない
- 敵が地面に落ちる途中、真上ライトでは影が一時見えるが着地前にMapに映る影が消えることがある

## 重要な判断

`shadowFocusPosition` と `Player Model` が同じなので、PlayerのLight NDCが `0, 0, 0.5` 付近で固定されるのは正常。

これは「範囲外で影が消えている」可能性を下げる材料。

次に疑うべきは以下。

1. Skinning Shadow VSの出力がDepthへ正しく書かれていない
2. Depthには書かれているが、通常描画側の影比較で拾えていない
3. SkinningモデルのShadow WVPまたはPaletteがShadowパス時点で想定とズレている
4. Depth previewの見え方だけではPlayer形状が判断できていない

## 次にやるべき調査

優先度順。

1. PlayerのShadow WVP後のAABB/NDC範囲をDebug表示する
2. PlayerのローカルAABB 8頂点を `UpdateShadowTransformGpuData()` 後のWVPで変換し、min/max NDCを出す
3. `SkinningShadowMap.VS.hlsl` を一時的に非スキニング化して、Playerの未スキニングメッシュがDepthに出るか確認する
4. Skinning Shadow VSでPalette参照が正しいか確認する
5. ShadowMap DebugのDepth previewを線形化・コントラスト調整して、Playerが本当にDepthにいないか見やすくする
6. 通常描画PS側で `shadowFactor` をDebug表示できるモードを作る

## 次の実装候補: Player Shadow AABB/NDC Debug

やるなら `ModelManager` のPlayer Shadow Debug構造体に以下を追加する。

- `bool hasShadowNdcBounds`
- `Vector3 shadowNdcMin`
- `Vector3 shadowNdcMax`
- `bool shadowNdcIntersectsClip`

計算場所候補。

- `Model::DrawShadow()` 内
- または `Model::CalculateWorldAABB()` を使って `ModelManager` 側で計算

ただしSkinningモデルはローカルAABBがアニメーション後の姿勢を完全には表さない可能性がある。

最初の切り分けには、以下で十分。

1. `Model::CalculateWorldAABB()` でワールドAABBを取得
2. AABB 8頂点を `lightViewProjection` で変換
3. min/max NDCを出す

この結果が範囲内なら、ShadowCamera範囲の問題ではない。

## 注意点

毎フレームLoggerを増やさないこと。

現在のDebugはImGui表示なのでOK。

コメント追加時はプロジェクトルールに従う。

- 通常コメントは `//`
- 関数コメントは Doxygen形式
- `/// @brief`
- `/// @param`
- `/// @return`
- 日本語で書く

## 直近のビルド結果

以下の構成でビルド成功済み。

- Configuration: `Debug`
- Platform: `x64`
- 警告: 0
- エラー: 0

使用したMSBuild。

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\amd64\MSBuild.exe" MadoEngine.sln /p:Configuration=Debug /p:Platform=x64 /m
```

## 主な関連ファイル

- `Engine/Render/Shadow/ShadowMap.h`
- `Engine/Render/Shadow/ShadowMap.cpp`
- `Engine/.Execution/Execution.h`
- `Engine/.Execution/Execution.cpp`
- `Engine/Shader/RootSignatureManager.cpp`
- `Engine/Render/PSO/PSOFactory.cpp`
- `Engine/Render/Object/3d/Model/Model.h`
- `Engine/Render/Object/3d/Model/Model.cpp`
- `Engine/Render/Object/3d/Model/ModelManager.h`
- `Engine/Render/Object/3d/Model/ModelManager.cpp`
- `Engine/Render/Object/3d/Model/InstancedModel.h`
- `Engine/Render/Object/3d/Model/InstancedModel.cpp`
- `Assets/Shader/Object3d/Shadow/ShadowMap.VS.hlsl`
- `Assets/Shader/Object3d/Shadow/SkinningShadowMap.VS.hlsl`
- `Assets/Shader/Object3d/Shadow/InstancedShadowMap.VS.hlsl`
- `Assets/Shader/Object3d/Model/Model.PS.hlsl`
- `Assets/Shader/Object3d/Model/SkinningModel.PS.hlsl`
- `Application/.SceneManager/Scene/Test.cpp`
- `Application/Object/Player/Player.cpp`

