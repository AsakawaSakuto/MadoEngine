#pragma once
#include "AnimationStruct.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

/// @brief 複数のAnimationClipを共有データとして管理するクラス
class AnimationSet {
public:

	/// @brief AnimationClipを登録
	/// @param name 登録名
	/// @param clip 登録対象のAnimationClip
	/// @return 登録に成功した場合はtrue
	bool AddClip(std::string name, AnimationClip clip);

	/// @brief 名前からAnimationClipを検索
	/// @param name 検索する登録名
	/// @return 見つかったAnimationClip、存在しない場合はnullptr
	const AnimationClip* FindClip(std::string_view name) const;

	/// @brief 標準再生するAnimationClipを設定
	/// @param name 標準再生する登録名
	/// @return 設定に成功した場合はtrue
	bool SetDefaultClip(std::string_view name);

	/// @brief 標準再生するAnimationClipを取得
	/// @return 標準再生するAnimationClip、存在しない場合はnullptr
	const AnimationClip* GetDefaultClip() const;

	/// @brief 標準再生するAnimationClip名を取得
	/// @return 標準再生するAnimationClip名
	const std::string& GetDefaultClipName() const { return defaultClipName_; }

	/// @brief 登録済みAnimationClip数を取得
	/// @return 登録済みAnimationClip数
	std::size_t GetClipCount() const { return clips_.size(); }

	/// @brief AnimationClipが未登録か判定
	/// @return 未登録の場合はtrue
	bool IsEmpty() const { return clips_.empty(); }

private:
	std::unordered_map<std::string, AnimationClip> clips_;
	std::string defaultClipName_;
};
