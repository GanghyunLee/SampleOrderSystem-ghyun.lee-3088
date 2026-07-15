// Requirements covered (docs/architecture.md §6/§7 - JsonValue):
//  - JsonValue represents boolean/number/string/array/object values and
//    exposes their type + underlying value without loss.
#include "gtest/gtest.h"
#include "JsonTestTypes.h"

TEST(JsonValue, BooleanValueStoresTypeAndValue) {
    JsonValue value(true);

    EXPECT_EQ(value.GetType(), JsonValue::Type::Boolean);
    EXPECT_TRUE(value.AsBool());
}

TEST(JsonValue, NumberValueStoresTypeAndValue) {
    JsonValue value(3.5);

    EXPECT_EQ(value.GetType(), JsonValue::Type::Number);
    EXPECT_DOUBLE_EQ(value.AsNumber(), 3.5);
}

TEST(JsonValue, StringValueStoresTypeAndValue) {
    JsonValue value(std::string("S-001"));

    EXPECT_EQ(value.GetType(), JsonValue::Type::String);
    EXPECT_EQ(value.AsString(), "S-001");
}

TEST(JsonValue, ArrayValueKeepsElementsInInsertionOrder) {
    JsonValue array(std::vector<JsonValue>{});
    array.Add(JsonValue(1.0));
    array.Add(JsonValue(2.0));

    ASSERT_EQ(array.GetType(), JsonValue::Type::Array);
    ASSERT_EQ(array.AsArray().size(), 2u);
    EXPECT_DOUBLE_EQ(array.AsArray()[0].AsNumber(), 1.0);
    EXPECT_DOUBLE_EQ(array.AsArray()[1].AsNumber(), 2.0);
}

TEST(JsonValue, ObjectValueStoresKeyValuePairsByName) {
    JsonValue object(std::map<std::string, JsonValue>{});
    object.Set("id", JsonValue(std::string("S-001")));
    object.Set("stock", JsonValue(10.0));

    ASSERT_EQ(object.GetType(), JsonValue::Type::Object);
    ASSERT_TRUE(object.Has("id"));
    ASSERT_TRUE(object.Has("stock"));
    EXPECT_EQ(object.Get("id").AsString(), "S-001");
    EXPECT_DOUBLE_EQ(object.Get("stock").AsNumber(), 10.0);
}

TEST(JsonValue, MissingKeyIsReportedByHas) {
    JsonValue object(std::map<std::string, JsonValue>{});
    object.Set("id", JsonValue(std::string("S-001")));

    EXPECT_FALSE(object.Has("stock"));
}
