#pragma once

#include "Utility/Camera/Camera.h"
#include "Utility/EditorManagementMode.h"
#include "Utility/Easing/EaseType.h"
#include "Utility/Handle/GenerationalHandle.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

struct CameraHandleTag;
using CameraHandle = MadoEngine::GenerationalHandle<CameraHandleTag>;

/// @brief Cameraから移動先へ切り替える方式
enum class CameraTransitionMode {
	Cut,
	Blend,
};

/// @brief Cameraが保持する移動先と切り替え設定
struct CameraTransitionSettings {
	CameraTransitionMode mode = CameraTransitionMode::Cut;
	CameraHandle destinationHandle{};
	float blendDuration = 0.5f;
	EaseType blendEaseType = EaseType::EaseInOutCubic;
};

/// @brief Cameraが保持するShake設定
struct CameraShakeSettings {
	float power = 0.25f;
	float duration = 0.5f;
	ShakeType type = ShakeType::XYZ;
};

/// @brief Scene内Cameraの所有と描画Cameraの切り替えを管理するクラス
class CameraManager {
public:
	static constexpr const char* kDefaultCameraJsonDirectory = "Assets/Json/Camera";

	CameraManager() = default;
	~CameraManager() = default;

	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;
	CameraManager(CameraManager&&) = delete;
	CameraManager& operator=(CameraManager&&) = delete;

	/// @brief 指定した型のCameraを生成して登録
	/// @tparam CameraType 生成するCameraの型
	/// @tparam Args Cameraコンストラクタへ渡す引数の型
	/// @param name Scene内でCameraを識別する一意な名前
	/// @param args Cameraコンストラクタへ渡す引数
	/// @return 登録したCameraのHandle、登録失敗時は無効Handle
	template<class CameraType, class... Args>
	[[nodiscard]] CameraHandle CreateCamera(const std::string& name, Args&&... args) {
		static_assert(std::is_base_of_v<Camera, CameraType>, "CameraTypeはCameraの派生型である必要があります");
		return RegisterCamera(
			name,
			std::make_unique<CameraType>(std::forward<Args>(args)...),
			MadoEngine::EditorManagementMode::RuntimeOnly
		);
	}

	/// @brief Editor管理対象として指定した型のCameraを生成して登録
	/// @tparam CameraType 生成するCameraの型
	/// @tparam Args Cameraコンストラクタへ渡す引数の型
	/// @param name Scene内でCameraを識別する一意な名前
	/// @param args Cameraコンストラクタへ渡す引数
	/// @return 登録したCameraのHandle、登録失敗時は無効Handle
	template<class CameraType, class... Args>
	[[nodiscard]] CameraHandle CreateEditorCamera(const std::string& name, Args&&... args) {
		static_assert(std::is_base_of_v<Camera, CameraType>, "CameraTypeはCameraの派生型である必要があります");
		return RegisterCamera(
			name,
			std::make_unique<CameraType>(std::forward<Args>(args)...),
			MadoEngine::EditorManagementMode::EditorManaged
		);
	}

	/// @brief HandleからCameraを取得
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はCamera、無効な場合はnullptr
	Camera* TryGetCamera(CameraHandle handle);

	/// @brief Handleから読み取り専用Cameraを取得
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はCamera、無効な場合はnullptr
	const Camera* TryGetCamera(CameraHandle handle) const;

	/// @brief Handleから指定した派生型のCameraを取得
	/// @tparam CameraType 取得するCameraの型
	/// @param handle 取得対象のHandle
	/// @return 型とHandleが有効な場合はCamera、無効な場合はnullptr
	template<class CameraType>
	CameraType* TryGetCamera(CameraHandle handle) {
		static_assert(std::is_base_of_v<Camera, CameraType>, "CameraTypeはCameraの派生型である必要があります");
		return dynamic_cast<CameraType*>(TryGetCamera(handle));
	}

	/// @brief Handleから指定した派生型の読み取り専用Cameraを取得
	/// @tparam CameraType 取得するCameraの型
	/// @param handle 取得対象のHandle
	/// @return 型とHandleが有効な場合はCamera、無効な場合はnullptr
	template<class CameraType>
	const CameraType* TryGetCamera(CameraHandle handle) const {
		static_assert(std::is_base_of_v<Camera, CameraType>, "CameraTypeはCameraの派生型である必要があります");
		return dynamic_cast<const CameraType*>(TryGetCamera(handle));
	}

	/// @brief 名前からCameraのHandleを検索
	/// @param name 検索するCamera名
	/// @return 見つかったCameraのHandle、見つからない場合は無効Handle
	[[nodiscard]] CameraHandle Find(const std::string& name) const;

	/// @brief 登録済みCameraのHandle一覧を取得
	/// @return 登録順のCamera Handle一覧
	[[nodiscard]] std::vector<CameraHandle> GetCameraHandles() const;

	/// @brief Camera名を取得
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はCamera名、無効な場合は空文字列
	[[nodiscard]] std::string GetCameraName(CameraHandle handle) const;

	/// @brief Camera名を変更
	/// @param handle 変更対象のHandle
	/// @param newName 変更後の一意なCamera名
	/// @return 名前を変更できた場合はtrue
	bool RenameCamera(CameraHandle handle, const std::string& newName);

	/// @brief Handleが有効なCameraを参照しているか確認
	/// @param handle 確認するHandle
	/// @return 有効なCameraを参照している場合はtrue
	[[nodiscard]] bool IsValid(CameraHandle handle) const;

	/// @brief CameraがEditor管理対象か確認
	/// @param handle 確認するHandle
	/// @return Editorから生成されたCameraの場合はtrue
	[[nodiscard]] bool IsEditorManaged(CameraHandle handle) const;

	/// @brief Cameraを登録解除して破棄
	/// @param handle 破棄するCameraのHandle
	/// @return 破棄できた場合はtrue
	bool DestroyCamera(CameraHandle handle);

	/// @brief 登録済みCameraをすべて破棄
	void Clear();

	/// @brief Scene名からCamera設定Jsonの既定パスを生成
	/// @param sceneName 保存対象のScene名
	/// @return Scene別Camera設定Jsonのパス
	[[nodiscard]] static std::filesystem::path CreateDefaultJsonPath(const std::string& sceneName);

	/// @brief Editor管理CameraとActive CameraをJsonへ保存
	/// @param filePath 保存先のJsonファイルパス
	/// @return 保存に成功した場合はtrue
	bool SaveToJson(const std::filesystem::path& filePath) const;

	/// @brief JsonからEditor管理CameraとActive Cameraを読み込み
	/// @param filePath 読み込み元のJsonファイルパス
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromJson(const std::filesystem::path& filePath);

	/// @brief 指定Cameraへ即時切り替え
	/// @param handle 切り替え先CameraのHandle
	/// @return 切り替えに成功した場合はtrue
	bool CutTo(CameraHandle handle);

	/// @brief 指定Cameraへ補間しながら切り替え
	/// @param handle 切り替え先CameraのHandle
	/// @param duration 補間時間
	/// @param easeType 補間へ適用するイージング
	/// @return 切り替えを開始できた場合はtrue
	bool BlendTo(
		CameraHandle handle,
		float duration,
		EaseType easeType = EaseType::EaseInOutCubic
	);

	/// @brief 登録Cameraと描画Cameraを更新
	/// @param deltaTime 前Frameからの経過時間
	void Update(float deltaTime);

	/// @brief 描画に使用するCameraを取得
	/// @return 描画Camera
	Camera& GetRenderCamera() { return renderCamera_; }

	/// @brief 描画に使用する読み取り専用Cameraを取得
	/// @return 描画Camera
	const Camera& GetRenderCamera() const { return renderCamera_; }

	/// @brief 現在有効なCameraのHandleを取得
	/// @return 有効CameraのHandle
	[[nodiscard]] CameraHandle GetActiveCameraHandle() const { return activeCameraHandle_; }

	/// @brief 補間先CameraのHandleを取得
	/// @return 補間中の場合は遷移先Handle、補間中ではない場合は無効Handle
	[[nodiscard]] CameraHandle GetBlendDestinationHandle() const { return blendDestinationHandle_; }

	/// @brief Camera補間の進行率を取得
	/// @return 0.0から1.0の補間率、補間中ではない場合は0.0
	[[nodiscard]] float GetBlendProgress() const;

	/// @brief Camera補間中か確認
	/// @return 補間中の場合はtrue
	[[nodiscard]] bool IsBlending() const { return isBlending_; }

	/// @brief Cameraが保持する遷移設定を取得
	/// @param handle 取得対象CameraのHandle
	/// @return 有効な場合は遷移設定、無効な場合は既定設定
	[[nodiscard]] CameraTransitionSettings GetTransitionSettings(CameraHandle handle) const;

	/// @brief Cameraが保持する遷移設定を更新
	/// @param handle 更新対象CameraのHandle
	/// @param settings 更新する遷移設定
	/// @return 設定を更新できた場合はtrue
	bool SetTransitionSettings(CameraHandle handle, const CameraTransitionSettings& settings);

	/// @brief Cameraが保持する設定を参照して移動先Cameraへ切り替え
	/// @param handle 遷移設定を使用するCameraのHandle
	/// @return Camera切り替えを開始できた場合はtrue
	bool ExecuteTransition(CameraHandle handle);

	/// @brief Cameraが保持するShake設定を取得
	/// @param handle 取得対象CameraのHandle
	/// @return 有効な場合はShake設定、無効な場合は既定設定
	[[nodiscard]] CameraShakeSettings GetShakeSettings(CameraHandle handle) const;

	/// @brief Cameraが保持するShake設定を更新
	/// @param handle 更新対象CameraのHandle
	/// @param settings 更新するShake設定
	/// @return 設定を更新できた場合はtrue
	bool SetShakeSettings(CameraHandle handle, const CameraShakeSettings& settings);

	/// @brief Cameraが保持する設定を参照してShakeを開始
	/// @param handle Shake設定を使用するCameraのHandle
	/// @return Shakeを開始できた場合はtrue
	bool ExecuteShake(CameraHandle handle);

private:
	struct CameraEntry {
		std::unique_ptr<Camera> camera;
		std::string name;
		CameraTransitionSettings transitionSettings;
		CameraShakeSettings shakeSettings;
		MadoEngine::EditorManagementMode managementMode = MadoEngine::EditorManagementMode::RuntimeOnly;
		uint32_t generation = 1;
	};

	/// @brief 所有権を受け取ったCameraを登録
	/// @param name Scene内でCameraを識別する一意な名前
	/// @param camera 登録するCamera
	/// @param managementMode Cameraの管理方法
	/// @return 登録したCameraのHandle、登録失敗時は無効Handle
	[[nodiscard]] CameraHandle RegisterCamera(
		const std::string& name,
		std::unique_ptr<Camera> camera,
		MadoEngine::EditorManagementMode managementMode
	);

	/// @brief HandleからCamera登録情報を取得
	/// @param handle 取得対象のHandle
	/// @return 有効な場合は登録情報、無効な場合はnullptr
	CameraEntry* TryGetEntry(CameraHandle handle);

	/// @brief Handleから読み取り専用Camera登録情報を取得
	/// @param handle 取得対象のHandle
	/// @return 有効な場合は登録情報、無効な場合はnullptr
	const CameraEntry* TryGetEntry(CameraHandle handle) const;

	/// @brief Camera補間状態を解除
	void ResetBlend();

	/// @brief Editor管理Cameraをすべて破棄
	void ClearEditorManagedCameras();

	/// @brief 二つのCamera状態から描画Cameraを生成
	/// @param source 補間開始Camera
	/// @param destination 補間終了Camera
	/// @param progress イージング適用済み補間率
	void UpdateBlendedCamera(const Camera& source, const Camera& destination, float progress);

	std::vector<CameraEntry> cameraEntries_;
	std::vector<uint32_t> freeIndices_;
	std::unordered_map<std::string, CameraHandle> cameraHandlesByName_;

	CameraHandle activeCameraHandle_{};
	CameraHandle blendDestinationHandle_{};
	Camera blendSourceSnapshot_;
	Camera renderCamera_;
	float blendDuration_ = 0.0f;
	float blendElapsedTime_ = 0.0f;
	EaseType blendEaseType_ = EaseType::Linear;
	bool isBlending_ = false;
};
