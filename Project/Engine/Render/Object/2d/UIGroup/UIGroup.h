#pragma once

#include "Math/TransformNode2D.h"
#include "Render/Object/ObjectHandle.h"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <variant>

class Sprite;

namespace MadoEngine {

class Text;

/// @brief Sprite・Text・子グループを名前付きで参照するUIグループ
/// @note 登録対象の所有権と描画順は既存Managerおよび呼び出し側の管理
class UIGroup final : public TransformNode2D {
public:

	/// @brief 空のUIグループを生成
	UIGroup() = default;
	/// @brief 登録対象を削除せず親子参照を解放
	~UIGroup() override = default;

	/// @brief Spriteをグループ内で一意な名前で登録
	/// @param name グループ内の名前
	/// @param handle 登録対象のSpriteハンドル
	/// @return 名前とハンドルが有効かつ未所属の対象を登録できた場合はtrue
	bool AddSprite(const std::string& name, SpriteHandle handle);
	/// @brief Textをグループ内で一意な名前で登録
	/// @param name グループ内の名前
	/// @param handle 登録対象のTextハンドル
	/// @return 名前とハンドルが有効かつ未所属の対象を登録できた場合はtrue
	bool AddText(const std::string& name, TextHandle handle);
	/// @brief 子グループを一意な名前で登録
	/// @param name グループ内の名前
	/// @param group 登録対象の子グループ
	/// @return 未所属の対象を循環なしで登録できた場合はtrue
	bool AddGroup(const std::string& name, UIGroup& group);

	/// @brief 名前に対応する有効な子Spriteを一時取得
	/// @param name グループ内の名前
	/// @return 有効な子Sprite、名前・型・所属が不一致の場合はnullptr
	Sprite* TryGetSprite(const std::string& name);
	/// @brief 名前に対応する有効な子Spriteを定数参照として一時取得
	/// @param name グループ内の名前
	/// @return 有効な子Sprite、名前・型・所属が不一致の場合はnullptr
	const Sprite* TryGetSprite(const std::string& name) const;
	/// @brief 名前に対応する有効な子Textを一時取得
	/// @param name グループ内の名前
	/// @return 有効な子Text、名前・型・所属が不一致の場合はnullptr
	Text* TryGetText(const std::string& name);
	/// @brief 名前に対応する有効な子Textを定数参照として一時取得
	/// @param name グループ内の名前
	/// @return 有効な子Text、名前・型・所属が不一致の場合はnullptr
	const Text* TryGetText(const std::string& name) const;
	/// @brief 名前に対応する有効な子グループを一時取得
	/// @param name グループ内の名前
	/// @return 有効な子グループ、名前・型・所属が不一致の場合はnullptr
	UIGroup* TryGetGroup(const std::string& name);
	/// @brief 名前に対応する有効な子グループを定数参照として一時取得
	/// @param name グループ内の名前
	/// @return 有効な子グループ、名前・型・所属が不一致の場合はnullptr
	const UIGroup* TryGetGroup(const std::string& name) const;

	/// @brief 対象を削除せずローカル値を維持した登録の解除
	/// @param name 解除するグループ内の名前
	/// @return 指定名の登録を解除できた場合はtrue
	bool Remove(const std::string& name);
	/// @brief 所属とローカル値を維持した要素名の変更
	/// @param name 現在の登録名
	/// @param newName 変更後の登録名
	/// @return 未使用の有効な名前へ変更できた場合はtrue
	bool RenameElement(const std::string& name, const std::string& newName);
	/// @brief 登録対象を削除せず全ての親子関係を解除
	void Clear();
	/// @brief 無効化済みの対象も含めた登録件数を取得
	/// @return 登録件数
	std::size_t GetElementCount() const { return elements_.size(); }
	/// @brief 無効化済みの対象も含めた登録名の有無を取得
	/// @param name 確認する登録名
	/// @return 登録名が存在する場合はtrue
	bool ContainsElement(const std::string& name) const { return elements_.contains(name); }

private:
	struct GroupReference {
		UIGroup* group = nullptr;
		std::weak_ptr<const LifetimeState> lifetime;
	};

	using Element = std::variant<SpriteHandle, TextHandle, GroupReference>;

	/// @brief ハンドルまたは寿命情報から有効な直接の子ノードを解決
	/// @param element 登録情報
	/// @return 有効な直接の子ノード、破棄または所属変更済みの場合はnullptr
	TransformNode2D* Resolve(const Element& element) const;

	std::unordered_map<std::string, Element> elements_;
};

} // namespace MadoEngine
