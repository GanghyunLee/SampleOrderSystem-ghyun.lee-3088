// Requirements covered (docs/architecture.md §6 - 데이터 영속성, §9 - JSON
// 저장/로드 왕복(round-trip) 시 데이터 무손실):
//  - Saving a JsonValue to a file and loading it back reproduces the same
//    scalar field values (string/number/bool).
//  - Saving a JsonValue containing an array of objects and loading it back
//    reproduces the same nested structure and values.
//  - Loading a file that does not exist fails gracefully (returns false)
//    instead of crashing.
#include "gtest/gtest.h"
#include "JsonTestTypes.h"

#include <filesystem>

namespace {

std::filesystem::path MakeTempJsonPath(const char* fileName) {
    return std::filesystem::temp_directory_path() / fileName;
}

} // namespace

TEST(SaveManager, SaveThenLoadRoundTripPreservesScalarFields) {
    const auto path = MakeTempJsonPath("sos_save_manager_scalar_roundtrip_test.json");
    std::filesystem::remove(path);

    JsonValue data(std::map<std::string, JsonValue>{});
    data.Set("orderId", JsonValue(std::string("ORD-20260416-0043")));
    data.Set("quantity", JsonValue(150.0));
    data.Set("released", JsonValue(false));

    ASSERT_TRUE(SaveManager::Save(path.string(), data));

    JsonValue loaded;
    ASSERT_TRUE(SaveManager::Load(path.string(), loaded));

    EXPECT_EQ(loaded.Get("orderId").AsString(), "ORD-20260416-0043");
    EXPECT_DOUBLE_EQ(loaded.Get("quantity").AsNumber(), 150.0);
    EXPECT_FALSE(loaded.Get("released").AsBool());

    std::filesystem::remove(path);
}

TEST(SaveManager, SaveThenLoadRoundTripPreservesArrayOfObjects) {
    const auto path = MakeTempJsonPath("sos_save_manager_array_roundtrip_test.json");
    std::filesystem::remove(path);

    JsonValue root(std::map<std::string, JsonValue>{});
    JsonValue samples(std::vector<JsonValue>{});

    JsonValue firstSample(std::map<std::string, JsonValue>{});
    firstSample.Set("id", JsonValue(std::string("S-001")));
    firstSample.Set("stock", JsonValue(20.0));
    samples.Add(firstSample);

    JsonValue secondSample(std::map<std::string, JsonValue>{});
    secondSample.Set("id", JsonValue(std::string("S-002")));
    secondSample.Set("stock", JsonValue(5.0));
    samples.Add(secondSample);

    root.Set("samples", samples);

    ASSERT_TRUE(SaveManager::Save(path.string(), root));

    JsonValue loaded;
    ASSERT_TRUE(SaveManager::Load(path.string(), loaded));

    const auto& loadedSamples = loaded.Get("samples").AsArray();
    ASSERT_EQ(loadedSamples.size(), 2u);
    EXPECT_EQ(loadedSamples[0].Get("id").AsString(), "S-001");
    EXPECT_DOUBLE_EQ(loadedSamples[0].Get("stock").AsNumber(), 20.0);
    EXPECT_EQ(loadedSamples[1].Get("id").AsString(), "S-002");
    EXPECT_DOUBLE_EQ(loadedSamples[1].Get("stock").AsNumber(), 5.0);

    std::filesystem::remove(path);
}

TEST(SaveManager, LoadOfMissingFileFailsGracefully) {
    const auto path = MakeTempJsonPath("sos_save_manager_file_that_does_not_exist.json");
    std::filesystem::remove(path);

    JsonValue loaded;
    EXPECT_FALSE(SaveManager::Load(path.string(), loaded));
}
