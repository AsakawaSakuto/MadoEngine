#include <nlohmann/json.hpp>
class ISerializable {
public:
    virtual ~ISerializable() = default;

    // 自身のデータをJSONに書き出す
    virtual void Serialize(nlohmann::json& outJson) const = 0;

    // JSONから自身のデータを読み込む
    virtual void Deserialize(const nlohmann::json& inJson) = 0;
};