#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/common/math.hpp>

#include <string>
#include <vector>
#include <map>

T_BEGIN_NAMESPACE
class Value {
public:
	enum class Type {
		EMPTY, INT, BOOL, FLOAT, STRING, BINARY, ARRAY, MAP
	};

	Value();
	explicit					Value(int aInt);
	explicit					Value(bool aBool);
	explicit					Value(float aFloat);
	explicit					Value(std::string_view aString);
	explicit					Value(std::vector<unsigned char> aBinary);
	explicit					Value(std::vector<Value> aArray);
	explicit					Value(std::map<std::string, Value> aMap);

	[[nodiscard]] bool			isEmpty() const;
	[[nodiscard]] bool			isInt() const;
	[[nodiscard]] bool			isBool() const;
	[[nodiscard]] bool			isFloat() const;
	[[nodiscard]] bool			isString() const;
	[[nodiscard]] bool			isBinary() const;
	[[nodiscard]] bool			isArray() const;
	[[nodiscard]] bool			isMap() const;

	[[nodiscard]] Type							getType() const;
	[[nodiscard]] int							getInt() const;
	[[nodiscard]] bool							getBool() const;
	[[nodiscard]] float							getFloat() const;
	[[nodiscard]] std::string					getString() const;
	[[nodiscard]] std::vector<unsigned char>	getBinary() const;
	[[nodiscard]] std::vector<Value>			getArray() const;
	[[nodiscard]] std::map<std::string, Value>	getMap() const;

	void						setInt(int aInt);
	void						setBool(bool aBool);
	void						setFloat(float aFloat);
	void						setString(const std::string& aString);
	void						setBinary(const std::vector<unsigned char>& aBinary);
	void						setArray(const std::vector<Value>& aArray);
	void						setMap(const std::map<std::string, Value>& aMap);
private:
	Type						mType;
	int							mIntValue;
	bool						mBoolValue;
	float						mFloatValue;
	std::string					mStringValue;
	std::vector<unsigned char>	mBinaryValue;
	std::vector<Value>			mArrayValue;
	std::map<std::string, Value>mMapValue;
};

class Asset {
public:
	T_DEFAULT_MOVE_COPY_CONSTRUCTOR(Asset)
	enum class Type {
		UNDEFINED, MESH, MODEL, CAMERA, LIGHT, IMAGE, MATERIAL, NODE
	};

	explicit									Asset(Type aType);
												Asset(Type aType, std::string_view aName);
	virtual										~Asset() = default;

	UUID										getUUID() const;
	[[nodiscard]] std::string_view				getName() const;
	[[nodiscard]] std::string_view				getFilepath() const;
	[[nodiscard]] Type							getAssetType() const;
	Value										getCustomProperty(const std::string& aKey);
	std::map<std::string, Value>*				getCustomPropertyMap();

	void										setName(std::string_view aName);
	void										setFilepath(std::string_view aFilepath);
	void										addCustomProperty(const std::string& aKey, const Value& aValue);
protected:
	Type										mAssetType;
	UUID										mUUID;
	std::string									mName;
	std::string									mFilepath;
	std::map<std::string, Value>				mCustomProperties;
};
T_END_NAMESPACE
