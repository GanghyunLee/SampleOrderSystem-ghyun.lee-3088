#include "JsonParser.h"

#include <cctype>
#include <cstring>

namespace {

void AppendUtf8(std::string& out, unsigned int codepoint) {
    if (codepoint <= 0x7F) {
        out += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        out += static_cast<char>(0xC0 | (codepoint >> 6));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        out += static_cast<char>(0xE0 | (codepoint >> 12));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text), pos_(0) {
    }

    bool Parse(JsonValue& out) {
        SkipWhitespace();
        if (!ParseValue(out)) return false;
        SkipWhitespace();
        return pos_ == text_.size();
    }

private:
    const std::string& text_;
    size_t pos_;

    void SkipWhitespace() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    bool Peek(char& c) const {
        if (pos_ >= text_.size()) return false;
        c = text_[pos_];
        return true;
    }

    bool ParseValue(JsonValue& out) {
        SkipWhitespace();
        char c;
        if (!Peek(c)) return false;
        switch (c) {
            case '{': return ParseObject(out);
            case '[': return ParseArray(out);
            case '"': {
                std::string s;
                if (!ParseString(s)) return false;
                out = JsonValue(s);
                return true;
            }
            case 't': return ParseLiteral("true", JsonValue(true), out);
            case 'f': return ParseLiteral("false", JsonValue(false), out);
            case 'n': return ParseLiteral("null", JsonValue(), out);
            default: return ParseNumber(out);
        }
    }

    bool ParseLiteral(const char* literal, const JsonValue& value, JsonValue& out) {
        size_t len = std::strlen(literal);
        if (text_.compare(pos_, len, literal) != 0) return false;
        pos_ += len;
        out = value;
        return true;
    }

    bool ParseString(std::string& out) {
        if (pos_ >= text_.size() || text_[pos_] != '"') return false;
        ++pos_;
        std::string result;
        while (true) {
            if (pos_ >= text_.size()) return false;
            char c = text_[pos_++];
            if (c == '"') break;
            if (c == '\\') {
                if (pos_ >= text_.size()) return false;
                char esc = text_[pos_++];
                switch (esc) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'u': {
                        if (pos_ + 4 > text_.size()) return false;
                        unsigned int codepoint = 0;
                        for (int i = 0; i < 4; ++i) {
                            char hexChar = text_[pos_ + i];
                            codepoint <<= 4;
                            if (hexChar >= '0' && hexChar <= '9') {
                                codepoint |= static_cast<unsigned int>(hexChar - '0');
                            } else if (hexChar >= 'a' && hexChar <= 'f') {
                                codepoint |= static_cast<unsigned int>(hexChar - 'a' + 10);
                            } else if (hexChar >= 'A' && hexChar <= 'F') {
                                codepoint |= static_cast<unsigned int>(hexChar - 'A' + 10);
                            } else {
                                return false;
                            }
                        }
                        pos_ += 4;
                        // 서로게이트 페어(BMP 밖 문자, \uD800~\uDFFF)는 0단계 범위 밖 — architecture.md §6 참고
                        AppendUtf8(result, codepoint);
                        break;
                    }
                    default: return false;
                }
            } else {
                result += c;
            }
        }
        out = result;
        return true;
    }

    bool ParseNumber(JsonValue& out) {
        size_t start = pos_;
        if (pos_ < text_.size() && (text_[pos_] == '-' || text_[pos_] == '+')) ++pos_;
        bool hasDigits = false;
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
            hasDigits = true;
        }
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
                ++pos_;
                hasDigits = true;
            }
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            size_t expStart = pos_;
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            bool expDigits = false;
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
                ++pos_;
                expDigits = true;
            }
            if (!expDigits) pos_ = expStart;
        }
        if (!hasDigits) return false;
        try {
            double value = std::stod(text_.substr(start, pos_ - start));
            out = JsonValue(value);
            return true;
        } catch (...) {
            return false;
        }
    }

    bool ParseArray(JsonValue& out) {
        ++pos_; // consume '['
        JsonValue arr(std::vector<JsonValue>{});
        SkipWhitespace();
        char c;
        if (Peek(c) && c == ']') {
            ++pos_;
            out = arr;
            return true;
        }
        while (true) {
            JsonValue element;
            if (!ParseValue(element)) return false;
            arr.Add(element);
            SkipWhitespace();
            if (!Peek(c)) return false;
            if (c == ',') {
                ++pos_;
                continue;
            }
            if (c == ']') {
                ++pos_;
                break;
            }
            return false;
        }
        out = arr;
        return true;
    }

    bool ParseObject(JsonValue& out) {
        ++pos_; // consume '{'
        JsonValue obj(std::map<std::string, JsonValue>{});
        SkipWhitespace();
        char c;
        if (Peek(c) && c == '}') {
            ++pos_;
            out = obj;
            return true;
        }
        while (true) {
            SkipWhitespace();
            std::string key;
            if (!ParseString(key)) return false;
            SkipWhitespace();
            if (!Peek(c) || c != ':') return false;
            ++pos_;
            JsonValue value;
            if (!ParseValue(value)) return false;
            obj.Set(key, value);
            SkipWhitespace();
            if (!Peek(c)) return false;
            if (c == ',') {
                ++pos_;
                continue;
            }
            if (c == '}') {
                ++pos_;
                break;
            }
            return false;
        }
        out = obj;
        return true;
    }
};

} // namespace

JsonValue JsonParser::Parse(const std::string& text) {
    JsonValue out;
    TryParse(text, out);
    return out;
}

bool JsonParser::TryParse(const std::string& text, JsonValue& out) {
    Parser parser(text);
    return parser.Parse(out);
}
