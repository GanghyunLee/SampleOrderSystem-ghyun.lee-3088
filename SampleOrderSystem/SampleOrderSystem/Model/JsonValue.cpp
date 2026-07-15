#include "JsonValue.h"

#include <charconv>

JsonValue::JsonValue() : type_(Type::Null) {
}

JsonValue::JsonValue(bool value) : type_(Type::Boolean), boolValue_(value) {
}

JsonValue::JsonValue(double value) : type_(Type::Number), numberValue_(value) {
}

JsonValue::JsonValue(const std::string& value) : type_(Type::String), stringValue_(value) {
}

JsonValue::JsonValue(const std::vector<JsonValue>& value) : type_(Type::Array), arrayValue_(value) {
}

JsonValue::JsonValue(const std::map<std::string, JsonValue>& value) : type_(Type::Object), objectValue_(value) {
}

JsonValue::Type JsonValue::GetType() const {
    return type_;
}

bool JsonValue::AsBool() const {
    return boolValue_;
}

double JsonValue::AsNumber() const {
    return numberValue_;
}

const std::string& JsonValue::AsString() const {
    return stringValue_;
}

const std::vector<JsonValue>& JsonValue::AsArray() const {
    return arrayValue_;
}

const std::map<std::string, JsonValue>& JsonValue::AsObject() const {
    return objectValue_;
}

void JsonValue::Add(const JsonValue& element) {
    type_ = Type::Array;
    arrayValue_.push_back(element);
}

void JsonValue::Set(const std::string& key, const JsonValue& value) {
    type_ = Type::Object;
    objectValue_[key] = value;
}

const JsonValue& JsonValue::Get(const std::string& key) const {
    static const JsonValue nullValue;
    auto it = objectValue_.find(key);
    if (it == objectValue_.end()) {
        return nullValue;
    }
    return it->second;
}

bool JsonValue::Has(const std::string& key) const {
    return objectValue_.find(key) != objectValue_.end();
}

namespace {

void AppendEscapedString(const std::string& value, std::string& out) {
    out += '"';
    for (char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            default: out += c; break;
        }
    }
    out += '"';
}

void AppendNumber(double value, std::string& out) {
    char buffer[64];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    out.append(buffer, result.ptr);
}

} // namespace

void JsonValue::SerializeTo(std::string& out) const {
    switch (type_) {
        case Type::Null:
            out += "null";
            break;
        case Type::Boolean:
            out += boolValue_ ? "true" : "false";
            break;
        case Type::Number:
            AppendNumber(numberValue_, out);
            break;
        case Type::String:
            AppendEscapedString(stringValue_, out);
            break;
        case Type::Array: {
            out += '[';
            bool first = true;
            for (const auto& element : arrayValue_) {
                if (!first) out += ',';
                first = false;
                element.SerializeTo(out);
            }
            out += ']';
            break;
        }
        case Type::Object: {
            out += '{';
            bool first = true;
            for (const auto& [key, value] : objectValue_) {
                if (!first) out += ',';
                first = false;
                AppendEscapedString(key, out);
                out += ':';
                value.SerializeTo(out);
            }
            out += '}';
            break;
        }
    }
}

std::string JsonValue::Serialize() const {
    std::string out;
    SerializeTo(out);
    return out;
}
