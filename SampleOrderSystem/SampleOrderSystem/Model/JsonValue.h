#pragma once

// JsonValue represents a JSON value (null/bool/number/string/array/object).
// See docs/architecture.md section 6 (data persistence) for intended usage.

#include <map>
#include <string>
#include <vector>

class JsonValue {
public:
    enum class Type { Null, Boolean, Number, String, Array, Object };

    JsonValue();
    explicit JsonValue(bool value);
    explicit JsonValue(double value);
    explicit JsonValue(const std::string& value);
    explicit JsonValue(const std::vector<JsonValue>& value);
    explicit JsonValue(const std::map<std::string, JsonValue>& value);

    Type GetType() const;

    bool AsBool() const;
    double AsNumber() const;
    const std::string& AsString() const;
    const std::vector<JsonValue>& AsArray() const;
    const std::map<std::string, JsonValue>& AsObject() const;

    // Mutators used to build arrays/objects incrementally.
    void Add(const JsonValue& element);
    void Set(const std::string& key, const JsonValue& value);

    const JsonValue& Get(const std::string& key) const;
    bool Has(const std::string& key) const;

    std::string Serialize() const;

private:
    Type type_ = Type::Null;
    bool boolValue_ = false;
    double numberValue_ = 0.0;
    std::string stringValue_;
    std::vector<JsonValue> arrayValue_;
    std::map<std::string, JsonValue> objectValue_;

    void SerializeTo(std::string& out) const;
};
