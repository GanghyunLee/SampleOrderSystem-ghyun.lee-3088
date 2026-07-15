// Requirements covered (docs/FEATURES/02-sample-management.md, cross-checked
// against docs/PRD.md section 7 / docs/architecture.md section 3.1):
//  1. Registering a new sample succeeds and the sample is retrievable via
//     FindAll with all its input fields intact.
//  2. A newly registered sample starts with stock == 0.
//  3. Registering a sample whose id already exists is rejected (duplicate
//     id), and no second entry is added to the repository.
//  4. Yield must satisfy 0 < yield <= 1; registration is rejected when
//     yield is zero, negative, or greater than 1.
//  5. Yield exactly 1.0 (upper boundary, inclusive) is accepted.
//  6. FindAll returns every registered sample (not just the first one).
//  7. SearchByName performs a substring (partial) match on the sample name,
//     returning matches and excluding non-matches, and returns an empty
//     result when nothing matches.
//  8. Registering a sample with an empty id is rejected (required-field
//     validation per FEATURES doc line 47).
//  9. Registering a sample with an empty name is rejected (required-field
//     validation per FEATURES doc line 47).
// 10. FindById returns true and the matching sample when the id exists
//     (direct unit test for docs/FEATURES/04-order-approval-rejection.md's
//     reliance on SampleRepository::FindById; docs/architecture.md section
//     3.1).
// 11. FindById returns false when the id does not exist.
// 12. AddStock increases the stock of the sample with the given id
//     (direct unit test; docs/FEATURES/04-order-approval-rejection.md line
//     20, "해당 수량만큼 재고를 차감" implies the counterpart stock-increase
//     primitive must behave correctly in isolation).
// 13. DecreaseStock reduces the stock of the sample with the given id by
//     the requested quantity under normal (sufficient-stock) conditions.
// 14. DecreaseStock clamps stock at 0 (never negative) when the requested
//     quantity exceeds the current stock (docs/architecture.md section 3.1,
//     "DecreaseStock never takes stock below 0").
//
// Note: sample names below are plain ASCII on purpose (not the Korean
// example names from the FEATURES doc) so this file's meaning does not
// depend on the compiler's assumed source encoding; substring matching
// behavior does not depend on the character set used.
#include "gtest/gtest.h"
#include "SampleTestTypes.h"

namespace {

Sample MakeSample(const std::string& id, const std::string& name,
                   double avgProductionTimeMin, double yield) {
    Sample sample;
    sample.id = id;
    sample.name = name;
    sample.avgProductionTimeMin = avgProductionTimeMin;
    sample.yield = yield;
    return sample;
}

} // namespace

TEST(SampleRepository, RegisterNewSampleReturnsTrue) {
    SampleRepository repo;

    EXPECT_TRUE(repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92)));
}

TEST(SampleRepository, FindAllAfterRegisterContainsAllInputFields) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    const auto samples = repo.FindAll();

    ASSERT_EQ(samples.size(), 1u);
    EXPECT_EQ(samples[0].id, "S-001");
    EXPECT_EQ(samples[0].name, "Silicon Wafer 8-inch");
    EXPECT_DOUBLE_EQ(samples[0].avgProductionTimeMin, 0.5);
    EXPECT_DOUBLE_EQ(samples[0].yield, 0.92);
}

TEST(SampleRepository, NewlyRegisteredSampleStartsWithZeroStock) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    const auto samples = repo.FindAll();

    ASSERT_EQ(samples.size(), 1u);
    EXPECT_EQ(samples[0].stock, 0);
}

TEST(SampleRepository, RegisterDuplicateIdReturnsFalse) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    EXPECT_FALSE(repo.Register(MakeSample("S-001", "Different Name", 0.3, 0.8)));
}

TEST(SampleRepository, RegisterDuplicateIdDoesNotAddSecondEntry) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    repo.Register(MakeSample("S-001", "Different Name", 0.3, 0.8));

    EXPECT_EQ(repo.FindAll().size(), 1u);
}

TEST(SampleRepository, RegisterWithZeroYieldReturnsFalse) {
    SampleRepository repo;

    EXPECT_FALSE(repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.0)));
}

TEST(SampleRepository, RegisterWithNegativeYieldReturnsFalse) {
    SampleRepository repo;

    EXPECT_FALSE(repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, -0.1)));
}

TEST(SampleRepository, RegisterWithYieldGreaterThanOneReturnsFalse) {
    SampleRepository repo;

    EXPECT_FALSE(repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 1.1)));
}

TEST(SampleRepository, RegisterWithYieldExactlyOneReturnsTrue) {
    SampleRepository repo;

    EXPECT_TRUE(repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 1.0)));
}

TEST(SampleRepository, FindAllReturnsEveryRegisteredSample) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    repo.Register(MakeSample("S-002", "GaN Epitaxial 4-inch", 0.3, 0.78));

    EXPECT_EQ(repo.FindAll().size(), 2u);
}

TEST(SampleRepository, SearchByNameReturnsPartialMatch) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    repo.Register(MakeSample("S-002", "GaN Epitaxial 4-inch", 0.3, 0.78));

    const auto results = repo.SearchByName("Wafer");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "S-001");
}

TEST(SampleRepository, SearchByNameExcludesNonMatchingSamples) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    repo.Register(MakeSample("S-002", "GaN Epitaxial 4-inch", 0.3, 0.78));

    const auto results = repo.SearchByName("Wafer");

    for (const auto& sample : results) {
        EXPECT_NE(sample.id, "S-002");
    }
}

TEST(SampleRepository, SearchByNameWithNoMatchReturnsEmptyResult) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    EXPECT_TRUE(repo.SearchByName("NoSuchName").empty());
}

TEST(SampleRepository, RegisterWithEmptyIdReturnsFalse) {
    SampleRepository repo;

    EXPECT_FALSE(repo.Register(MakeSample("", "Silicon Wafer 8-inch", 0.5, 0.92)));
}

TEST(SampleRepository, RegisterWithEmptyNameReturnsFalse) {
    SampleRepository repo;

    EXPECT_FALSE(repo.Register(MakeSample("S-001", "", 0.5, 0.92)));
}

TEST(SampleRepository, FindByIdReturnsTrueAndMatchingSampleWhenIdExists) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    Sample outSample;
    ASSERT_TRUE(repo.FindById("S-001", outSample));
    EXPECT_EQ(outSample.id, "S-001");
    EXPECT_EQ(outSample.name, "Silicon Wafer 8-inch");
}

TEST(SampleRepository, FindByIdReturnsFalseWhenIdDoesNotExist) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    Sample outSample;
    EXPECT_FALSE(repo.FindById("S-NO-SUCH-ID", outSample));
}

TEST(SampleRepository, AddStockIncreasesStockForExistingSample) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    ASSERT_TRUE(repo.AddStock("S-001", 50));

    Sample outSample;
    ASSERT_TRUE(repo.FindById("S-001", outSample));
    EXPECT_EQ(outSample.stock, 50);
}

TEST(SampleRepository, DecreaseStockReducesStockByRequestedQuantity) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    repo.AddStock("S-001", 100);

    ASSERT_TRUE(repo.DecreaseStock("S-001", 30));

    Sample outSample;
    ASSERT_TRUE(repo.FindById("S-001", outSample));
    EXPECT_EQ(outSample.stock, 70);
}

TEST(SampleRepository, DecreaseStockClampsAtZeroWhenQuantityExceedsCurrentStock) {
    SampleRepository repo;
    repo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    repo.AddStock("S-001", 20);

    repo.DecreaseStock("S-001", 50);

    Sample outSample;
    ASSERT_TRUE(repo.FindById("S-001", outSample));
    EXPECT_EQ(outSample.stock, 0);
}
