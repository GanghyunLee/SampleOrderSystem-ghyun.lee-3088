// Requirements covered (docs/FEATURES/03-order-registration.md, cross-checked
// against docs/PRD.md section 5 / docs/architecture.md sections 3.2 and 5):
//  1. Registering an order for a sample id that exists in SampleRepository
//     succeeds.
//  2. Registering an order for a sample id that does NOT exist in
//     SampleRepository is rejected (FEATURES doc line 31).
//  3. Order quantity must be >= 1: quantity == 0 is rejected.
//  4. Order quantity must be >= 1: negative quantity is rejected.
//  5. Customer name must not be blank: empty string is rejected.
//  6. Customer name must not be blank: whitespace-only string is rejected.
//  7. A successfully registered order's initial status is RESERVED
//     (docs/PRD.md section 5).
//  8. The auto-issued order number matches the "ORD-YYYYMMDD-NNNN" format
//     (FEATURES doc line 25), with today's date as YYYYMMDD.
//  9. Order numbers issued for two orders registered on the same run are
//     distinct and sequential (NNNN increments).
//  10. Registration succeeds even when requested quantity exceeds the
//      sample's current stock (no stock check/deduction at registration
//      time -- that happens at approval, FEATURES doc line 27).
//  11. Registration succeeds even when the sample's stock is exactly zero.
//  12. FindAll returns every registered order (not just the first one),
//      with all input fields intact.
//  13. A rejected registration attempt does not add any entry to FindAll.
//  14. JSON round-trip via ToJson()/FromJson() (combined with SaveManager
//      Save/Load, mirroring the persistence pattern already exercised by
//      SaveManagerTest.cpp) preserves every field of a registered order,
//      including its RESERVED status.
#include "gtest/gtest.h"
#include "OrderTestTypes.h"
#include "SampleTestTypes.h"

#include <filesystem>
#include <chrono>
#include <cstdio>
#include <regex>

#include "../Model/SaveManager.h"

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

std::string TodayAsYyyymmdd() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTimeT = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &nowTimeT);
#else
    localtime_r(&nowTimeT, &tmBuf);
#endif
    char buf[16] = {0};
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d",
                  tmBuf.tm_year + 1900, tmBuf.tm_mon + 1, tmBuf.tm_mday);
    return std::string(buf);
}

} // namespace

TEST(OrderRepository, RegisterWithExistingSampleIdReturnsTrue) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    OrderRepository orderRepo;

    Order outOrder;
    EXPECT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, outOrder));
}

TEST(OrderRepository, RegisterWithUnregisteredSampleIdReturnsFalse) {
    SampleRepository sampleRepo; // no samples registered
    OrderRepository orderRepo;

    Order outOrder;
    EXPECT_FALSE(orderRepo.Register(sampleRepo, "S-999", "Acme Corp", 10, outOrder));
}

TEST(OrderRepository, RegisterWithZeroQuantityReturnsFalse) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    OrderRepository orderRepo;

    Order outOrder;
    EXPECT_FALSE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 0, outOrder));
}

TEST(OrderRepository, RegisterWithNegativeQuantityReturnsFalse) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    OrderRepository orderRepo;

    Order outOrder;
    EXPECT_FALSE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", -5, outOrder));
}

TEST(OrderRepository, RegisterWithEmptyCustomerNameReturnsFalse) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    OrderRepository orderRepo;

    Order outOrder;
    EXPECT_FALSE(orderRepo.Register(sampleRepo, "S-001", "", 10, outOrder));
}

TEST(OrderRepository, RegisterWithWhitespaceOnlyCustomerNameReturnsFalse) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    OrderRepository orderRepo;

    Order outOrder;
    EXPECT_FALSE(orderRepo.Register(sampleRepo, "S-001", "   ", 10, outOrder));
}

TEST(OrderRepository, SuccessfullyRegisteredOrderStartsAsReserved) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    OrderRepository orderRepo;

    Order outOrder;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, outOrder));
    EXPECT_EQ(outOrder.status, OrderStatus::RESERVED);
}

TEST(OrderRepository, GeneratedOrderIdMatchesOrdYyyymmddNnnnFormatWithTodaysDate) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    OrderRepository orderRepo;

    Order outOrder;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, outOrder));

    const std::string expectedPrefix = "ORD-" + TodayAsYyyymmdd() + "-";
    ASSERT_GE(outOrder.orderId.size(), expectedPrefix.size());
    EXPECT_EQ(outOrder.orderId.substr(0, expectedPrefix.size()), expectedPrefix);

    static const std::regex fullFormat(R"(^ORD-\d{8}-\d{4}$)");
    EXPECT_TRUE(std::regex_match(outOrder.orderId, fullFormat));
}

TEST(OrderRepository, TwoOrdersRegisteredInSameRunGetDistinctSequentialOrderIds) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    OrderRepository orderRepo;

    Order first;
    Order second;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, first));
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Beta Inc", 5, second));

    EXPECT_NE(first.orderId, second.orderId);

    // Trailing 4-digit sequence number must increment.
    const int firstSeq = std::stoi(first.orderId.substr(first.orderId.size() - 4));
    const int secondSeq = std::stoi(second.orderId.substr(second.orderId.size() - 4));
    EXPECT_EQ(secondSeq, firstSeq + 1);
}

TEST(OrderRepository, RegisterSucceedsEvenWhenQuantityExceedsCurrentStock) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    // Newly registered sample starts with stock == 0 (see
    // SampleRepositoryTest.cpp), so any positive quantity exceeds stock.
    OrderRepository orderRepo;

    Order outOrder;
    EXPECT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 999999, outOrder));
}

TEST(OrderRepository, RegisterSucceedsWhenSampleStockIsExactlyZero) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    const auto samples = sampleRepo.FindAll();
    ASSERT_EQ(samples.size(), 1u);
    ASSERT_EQ(samples[0].stock, 0);

    OrderRepository orderRepo;
    Order outOrder;
    EXPECT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 1, outOrder));
}

TEST(OrderRepository, FindAllReturnsEveryRegisteredOrderWithFieldsIntact) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    sampleRepo.Register(MakeSample("S-002", "GaN Epitaxial 4-inch", 0.3, 0.78));
    OrderRepository orderRepo;

    Order first;
    Order second;
    orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, first);
    orderRepo.Register(sampleRepo, "S-002", "Beta Inc", 20, second);

    const auto orders = orderRepo.FindAll();

    ASSERT_EQ(orders.size(), 2u);
    EXPECT_EQ(orders[0].sampleId, "S-001");
    EXPECT_EQ(orders[0].customerName, "Acme Corp");
    EXPECT_EQ(orders[0].quantity, 10);
    EXPECT_EQ(orders[1].sampleId, "S-002");
    EXPECT_EQ(orders[1].customerName, "Beta Inc");
    EXPECT_EQ(orders[1].quantity, 20);
}

TEST(OrderRepository, RejectedRegistrationDoesNotAddEntryToFindAll) {
    SampleRepository sampleRepo; // no samples registered
    OrderRepository orderRepo;

    Order outOrder;
    orderRepo.Register(sampleRepo, "S-999", "Acme Corp", 10, outOrder);

    EXPECT_TRUE(orderRepo.FindAll().empty());
}

TEST(OrderRepository, JsonRoundTripThroughSaveManagerPreservesAllOrderFields) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    OrderRepository orderRepo;

    Order registered;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 200, registered));

    const auto path = std::filesystem::temp_directory_path() /
        "sos_order_repository_roundtrip_test.json";
    std::filesystem::remove(path);

    ASSERT_TRUE(SaveManager::Save(path.string(), orderRepo.ToJson()));

    JsonValue loadedJson;
    ASSERT_TRUE(SaveManager::Load(path.string(), loadedJson));

    OrderRepository reloadedRepo;
    reloadedRepo.FromJson(loadedJson);

    const auto reloadedOrders = reloadedRepo.FindAll();
    ASSERT_EQ(reloadedOrders.size(), 1u);
    EXPECT_EQ(reloadedOrders[0].orderId, registered.orderId);
    EXPECT_EQ(reloadedOrders[0].sampleId, "S-001");
    EXPECT_EQ(reloadedOrders[0].customerName, "Acme Corp");
    EXPECT_EQ(reloadedOrders[0].quantity, 200);
    EXPECT_EQ(reloadedOrders[0].status, OrderStatus::RESERVED);

    std::filesystem::remove(path);
}

// Requirement 15 (docs/FEATURES/03-order-registration.md line 37, "생성된
// 주문은 애플리케이션 재시작 후에도 유지되어야 한다", cross-checked against the
// auto-issued "ORD-YYYYMMDD-NNNN" sequential numbering rule on line 25):
// after a save/reload cycle that simulates an application restart, the
// order-number sequence counter must also be restored so a newly
// registered order on the same day is issued a sequence number that does
// not collide with any already-persisted order number.
TEST(OrderRepository, OrderSequenceCounterSurvivesRestartAndAvoidsCollision) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    OrderRepository firstRunRepo;
    Order firstOrder;
    ASSERT_TRUE(firstRunRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, firstOrder));

    const auto path = std::filesystem::temp_directory_path() /
        "sos_order_repository_sequence_restart_test.json";
    std::filesystem::remove(path);

    ASSERT_TRUE(SaveManager::Save(path.string(), firstRunRepo.ToJson()));

    JsonValue loadedJson;
    ASSERT_TRUE(SaveManager::Load(path.string(), loadedJson));

    // Simulates restarting the application: a brand-new OrderRepository
    // instance loads the persisted state.
    OrderRepository secondRunRepo;
    secondRunRepo.FromJson(loadedJson);

    Order secondOrder;
    ASSERT_TRUE(secondRunRepo.Register(sampleRepo, "S-001", "Beta Inc", 5, secondOrder));

    // The newly issued order number must not collide with the one issued
    // (and already persisted) before the simulated restart.
    EXPECT_NE(secondOrder.orderId, firstOrder.orderId);

    const int firstSeq = std::stoi(firstOrder.orderId.substr(firstOrder.orderId.size() - 4));
    const int secondSeq = std::stoi(secondOrder.orderId.substr(secondOrder.orderId.size() - 4));
    EXPECT_EQ(secondSeq, firstSeq + 1);

    std::filesystem::remove(path);
}
