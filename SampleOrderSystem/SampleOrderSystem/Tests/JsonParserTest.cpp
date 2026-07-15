// Requirements covered (docs/architecture.md §6/§7 - JsonParser, §9 - JSON
// round-trip without data loss):
//  - Parsing a flat JSON object recovers each field's type and value.
//  - Parsing nested arrays of objects recovers structure and values.
//  - Malformed JSON is rejected via a failure return value, not a crash.
//  - Serialize() -> Parse() round-trip preserves data (no loss).
#include "gtest/gtest.h"
#include "JsonTestTypes.h"

TEST(JsonParser, ParsesFlatObjectFields) {
    JsonValue value = JsonParser::Parse(R"({"id":"S-001","stock":10,"active":true})");

    ASSERT_EQ(value.GetType(), JsonValue::Type::Object);
    EXPECT_EQ(value.Get("id").AsString(), "S-001");
    EXPECT_DOUBLE_EQ(value.Get("stock").AsNumber(), 10.0);
    EXPECT_TRUE(value.Get("active").AsBool());
}

TEST(JsonParser, ParsesNestedArrayOfObjects) {
    JsonValue value = JsonParser::Parse(
        R"({"orders":[{"orderId":"ORD-1","quantity":5},{"orderId":"ORD-2","quantity":7}]})");

    ASSERT_EQ(value.GetType(), JsonValue::Type::Object);
    const auto& orders = value.Get("orders").AsArray();
    ASSERT_EQ(orders.size(), 2u);
    EXPECT_EQ(orders[0].Get("orderId").AsString(), "ORD-1");
    EXPECT_DOUBLE_EQ(orders[0].Get("quantity").AsNumber(), 5.0);
    EXPECT_EQ(orders[1].Get("orderId").AsString(), "ORD-2");
    EXPECT_DOUBLE_EQ(orders[1].Get("quantity").AsNumber(), 7.0);
}

TEST(JsonParser, InvalidJsonReturnsFalseInsteadOfThrowingOrCrashing) {
    JsonValue out;

    bool parsedOk = JsonParser::TryParse("{ this is not valid json", out);

    EXPECT_FALSE(parsedOk);
}

TEST(JsonParser, SerializeThenParseRoundTripPreservesAllFieldValues) {
    JsonValue original(std::map<std::string, JsonValue>{});
    original.Set("sampleId", JsonValue(std::string("S-042")));
    original.Set("yield", JsonValue(0.85));
    original.Set("stock", JsonValue(12.0));
    original.Set("inStockYn", JsonValue(true));

    std::string serialized = original.Serialize();
    JsonValue reparsed = JsonParser::Parse(serialized);

    ASSERT_EQ(reparsed.GetType(), JsonValue::Type::Object);
    EXPECT_EQ(reparsed.Get("sampleId").AsString(), "S-042");
    EXPECT_DOUBLE_EQ(reparsed.Get("yield").AsNumber(), 0.85);
    EXPECT_DOUBLE_EQ(reparsed.Get("stock").AsNumber(), 12.0);
    EXPECT_TRUE(reparsed.Get("inStockYn").AsBool());
}
