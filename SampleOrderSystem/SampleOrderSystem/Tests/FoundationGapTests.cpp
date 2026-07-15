// Additional tests closing GAPs flagged by spec-verifier for Stage 0
// ("foundation"), against docs/architecture.md sections 2/6/7/9. These are
// added alongside (never replacing) JsonValueTest.cpp / JsonParserTest.cpp /
// SaveManagerTest.cpp.
//
// Requirements covered:
//  1. JsonValue::Get() with a missing key returns a well-defined Null value
//     instead of crashing or invoking undefined behavior.
//  2. A default-constructed JsonValue() reports Type::Null.
//  3. Serialize() -> Parse() round-trip preserves strings containing
//     characters that require JSON escaping (quote, backslash, newline, tab).
//  4. Serialize() -> Parse() round-trip preserves negative numbers and
//     exponent-notation numbers.
//  5. SaveManager::Save() to a path whose parent directory does not yet
//     exist auto-creates that directory and succeeds (docs/architecture.md
//     section 6: SaveManager::Save creates the missing parent directory
//     of the target file before writing, per the Stage-0 spec decision).
//  6. JSON parsing of a "\uXXXX" unicode escape restores the intended
//     (non-ASCII) character losslessly, without substituting '?'.
//  7. Serialize() -> Parse() round-trip preserves non-ASCII (UTF-8) string
//     content even without going through the "\uXXXX" escape path.
//
// Note: all string literals below use explicit \xNN / \uXXXX escapes
// (rather than literal non-ASCII source characters) so the test file's
// meaning does not depend on the compiler's assumed source encoding.
#include "gtest/gtest.h"
#include "JsonTestTypes.h"

#include <filesystem>
#include <fstream>
#include <sstream>

TEST(JsonValue, GetWithMissingKeyReturnsNullTypeInsteadOfCrashing) {
    JsonValue object(std::map<std::string, JsonValue>{});
    object.Set("id", JsonValue(std::string("S-001")));

    const JsonValue& missing = object.Get("doesNotExist");

    EXPECT_EQ(missing.GetType(), JsonValue::Type::Null);
}

TEST(JsonValue, DefaultConstructedValueIsNullType) {
    JsonValue value;

    EXPECT_EQ(value.GetType(), JsonValue::Type::Null);
}

TEST(JsonParser, SerializeThenParseRoundTripPreservesEscapedStringCharacters) {
    const std::string original = "line1\nline2\ttab\"quote\"\\backslash\\";

    JsonValue value(std::map<std::string, JsonValue>{});
    value.Set("note", JsonValue(original));

    std::string serialized = value.Serialize();
    JsonValue reparsed = JsonParser::Parse(serialized);

    ASSERT_EQ(reparsed.GetType(), JsonValue::Type::Object);
    EXPECT_EQ(reparsed.Get("note").AsString(), original);
}

TEST(JsonParser, SerializeThenParseRoundTripPreservesNegativeAndExponentNumbers) {
    JsonValue value(std::map<std::string, JsonValue>{});
    value.Set("negative", JsonValue(-5.0));
    value.Set("exponent", JsonValue(1.5e3));

    std::string serialized = value.Serialize();
    JsonValue reparsed = JsonParser::Parse(serialized);

    ASSERT_EQ(reparsed.GetType(), JsonValue::Type::Object);
    EXPECT_DOUBLE_EQ(reparsed.Get("negative").AsNumber(), -5.0);
    EXPECT_DOUBLE_EQ(reparsed.Get("exponent").AsNumber(), 1500.0);
}

TEST(SaveManager, SaveCreatesMissingParentDirectoryThenSucceeds) {
    const auto path = std::filesystem::temp_directory_path() /
        "sos_save_manager_nonexistent_parent_dir_test" / "store.json";
    // Precondition: ensure the parent directory truly does not exist.
    std::filesystem::remove_all(path.parent_path());
    ASSERT_FALSE(std::filesystem::exists(path.parent_path()));

    JsonValue data(std::map<std::string, JsonValue>{});
    data.Set("orderId", JsonValue(std::string("ORD-1")));

    // docs/architecture.md section 6: a missing parent directory is created
    // automatically rather than treated as a save failure.
    EXPECT_TRUE(SaveManager::Save(path.string(), data));
    EXPECT_TRUE(std::filesystem::exists(path));

    JsonValue loaded;
    ASSERT_TRUE(SaveManager::Load(path.string(), loaded));
    ASSERT_EQ(loaded.GetType(), JsonValue::Type::Object);
    EXPECT_EQ(loaded.Get("orderId").AsString(), "ORD-1");

    std::filesystem::remove_all(path.parent_path());
}

TEST(JsonParser, ParsesUnicodeEscapeIntoLosslessNonAsciiCharacter) {
    // U+AC00 (the Hangul syllable commonly romanized "ga") encoded as a JSON
    // \uXXXX escape must be restored to its exact UTF-8 byte sequence
    // (0xEA 0xB0 0x80), not a '?' placeholder or a dropped character.
    const std::string json = "{\"name\":\"\\uAC00\"}";

    JsonValue parsed = JsonParser::Parse(json);

    ASSERT_EQ(parsed.GetType(), JsonValue::Type::Object);
    const std::string expectedUtf8 = "\xEA\xB0\x80"; // U+AC00 in UTF-8
    EXPECT_EQ(parsed.Get("name").AsString(), expectedUtf8);
}

TEST(JsonParser, SerializeThenParseRoundTripPreservesNonAsciiUtf8String) {
    // A three-syllable Korean given name, as raw UTF-8 bytes, set directly
    // on a JsonValue without going through a "\uXXXX" escape.
    const std::string original =
        "\xED\x99\x8D\xEA\xB8\xB8\xEB\x8F\x99"; // three Hangul syllables

    JsonValue value(std::map<std::string, JsonValue>{});
    value.Set("customerName", JsonValue(original));

    std::string serialized = value.Serialize();
    JsonValue reparsed = JsonParser::Parse(serialized);

    ASSERT_EQ(reparsed.GetType(), JsonValue::Type::Object);
    EXPECT_EQ(reparsed.Get("customerName").AsString(), original);
}
