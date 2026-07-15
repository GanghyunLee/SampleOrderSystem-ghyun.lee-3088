// Requirements covered (docs/FEATURES/04-order-approval-rejection.md,
// cross-checked against docs/PRD.md section 5 / section 7, and
// docs/architecture.md sections 3.3, 4 and 5):
//
//  1. Approving a RESERVED order whose sample stock is sufficient (stock >
//     quantity) transitions the order to CONFIRMED (FEATURES doc line 20).
//  2. Approving with sufficient stock decreases the sample's stock by the
//     order quantity (FEATURES doc line 20).
//  3. The "sufficient" boundary is inclusive: stock exactly equal to the
//     order quantity is treated as sufficient -> CONFIRMED, stock becomes 0
//     (FEATURES doc line 20, "재고가 주문 수량 이상").
//  4. Approving a RESERVED order whose sample stock is insufficient (stock <
//     quantity) transitions the order to PRODUCING (FEATURES doc line 21).
//  5. Approving with insufficient stock fully consumes the existing stock
//     down to 0 (FEATURES doc line 24, recommended approach that this
//     project adopts).
//  6. Approving with insufficient stock computes shortage = quantity - stock
//     (FEATURES doc line 23).
//  7. Approving with insufficient stock enqueues a ProductionJob (into the
//     ProductionQueue) carrying the correct orderId/sampleId/shortage
//     (FEATURES doc line 21; docs/architecture.md section 3.3/5).
//  8. actualQuantity on the enqueued job equals ceil(shortage / yield)
//     (docs/PRD.md section 7; docs/architecture.md section 3.3).
//  9. totalTimeMin on the enqueued job equals avgProductionTimeMin *
//     actualQuantity (docs/architecture.md section 3.3).
// 10. Multiple insufficient-stock approvals enqueue ProductionJobs in FIFO
//     order (docs/PRD.md section 7, "생산 큐 스케줄링: FIFO").
// 11. Rejecting a RESERVED order transitions it to REJECTED (FEATURES doc
//     line 30).
// 12. Approving an order that is not RESERVED (already processed) is
//     refused and leaves its status unchanged (FEATURES doc line 43).
// 13. Rejecting an order that is not RESERVED (already processed) is
//     refused and leaves its status unchanged (FEATURES doc line 43).
// 14. Approving a nonexistent order id fails (FEATURES doc line 44).
// 15. Rejecting a nonexistent order id fails (FEATURES doc line 44).
// 16. Listing orders awaiting approval only returns RESERVED orders
//     (FEATURES doc lines 9-11, "접수된 주문 목록").
// 17. Approving an order already in CONFIRMED status is refused and leaves
//     its status unchanged (FEATURES doc line 43 - additional non-RESERVED
//     source state beyond REJECTED).
// 18. Approving an order already in PRODUCING status is refused and leaves
//     its status unchanged (FEATURES doc line 43).
// 19. Rejecting an order already in REJECTED status is refused and leaves
//     its status unchanged (FEATURES doc line 43 - additional non-RESERVED
//     source state beyond CONFIRMED).
// 20. Rejecting an order already in PRODUCING status is refused and leaves
//     its status unchanged (FEATURES doc line 43).
#include "gtest/gtest.h"
#include "OrderTestTypes.h"
#include "SampleTestTypes.h"
#include "ProductionTestTypes.h"

#include <algorithm>
#include <cmath>

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

Order FindOrderById(const OrderRepository& orderRepo, const std::string& orderId) {
    const auto orders = orderRepo.FindAll();
    const auto it = std::find_if(orders.begin(), orders.end(),
        [&](const Order& order) { return order.orderId == orderId; });
    return (it != orders.end()) ? *it : Order{};
}

Sample FindSampleById(const SampleRepository& sampleRepo, const std::string& sampleId) {
    Sample outSample;
    sampleRepo.FindById(sampleId, outSample);
    return outSample;
}

} // namespace

TEST(OrderApproval, SufficientStockTransitionsReservedToConfirmed) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    sampleRepo.AddStock("S-001", 500);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 300, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::CONFIRMED);
}

TEST(OrderApproval, SufficientStockDecreasesSampleStockByOrderQuantity) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    sampleRepo.AddStock("S-001", 500);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 300, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    EXPECT_EQ(FindSampleById(sampleRepo, "S-001").stock, 200);
}

TEST(OrderApproval, StockExactlyEqualToQuantityIsTreatedAsSufficient) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    sampleRepo.AddStock("S-001", 300);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 300, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::CONFIRMED);
    EXPECT_EQ(FindSampleById(sampleRepo, "S-001").stock, 0);
}

TEST(OrderApproval, InsufficientStockTransitionsReservedToProducing) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-003", "SiC Power Substrate 6-inch", 2.0, 0.9));
    sampleRepo.AddStock("S-003", 30);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-003", "Samsung Foundry", 200, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::PRODUCING);
}

TEST(OrderApproval, InsufficientStockConsumesExistingStockDownToZero) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-003", "SiC Power Substrate 6-inch", 2.0, 0.9));
    sampleRepo.AddStock("S-003", 30);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-003", "Samsung Foundry", 200, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    EXPECT_EQ(FindSampleById(sampleRepo, "S-003").stock, 0);
}

TEST(OrderApproval, InsufficientStockEnqueuesJobWithCorrectShortage) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-003", "SiC Power Substrate 6-inch", 2.0, 0.9));
    sampleRepo.AddStock("S-003", 30);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-003", "Samsung Foundry", 200, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    ASSERT_FALSE(queue.Empty());
    EXPECT_EQ(queue.Front().shortage, 170); // 200 - 30
}

TEST(OrderApproval, InsufficientStockEnqueuesJobWithCorrectOrderAndSampleIds) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-003", "SiC Power Substrate 6-inch", 2.0, 0.9));
    sampleRepo.AddStock("S-003", 30);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-003", "Samsung Foundry", 200, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    ASSERT_FALSE(queue.Empty());
    EXPECT_EQ(queue.Front().orderId, order.orderId);
    EXPECT_EQ(queue.Front().sampleId, "S-003");
}

TEST(OrderApproval, ActualQuantityIsCeilOfShortageDividedByYield) {
    // shortage = 10, yield = 0.4 -> ceil(10 / 0.4) = ceil(25.0) = 25
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-010", "GaN Epitaxial 4-inch", 3.0, 0.4));
    sampleRepo.AddStock("S-010", 0);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-010", "Beta Inc", 10, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    ASSERT_FALSE(queue.Empty());
    EXPECT_EQ(queue.Front().actualQuantity, 25);
}

TEST(OrderApproval, ActualQuantityRoundsUpWhenDivisionIsNotExact) {
    // shortage = 7, yield = 0.9 -> 7/0.9 = 7.777..., ceil = 8
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-011", "GaN Epitaxial 4-inch", 3.0, 0.9));
    sampleRepo.AddStock("S-011", 0);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-011", "Beta Inc", 7, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    ASSERT_FALSE(queue.Empty());
    EXPECT_EQ(queue.Front().actualQuantity, 8);
}

TEST(OrderApproval, TotalTimeMinIsAvgProductionTimeTimesActualQuantity) {
    // shortage = 10, yield = 0.4 -> actualQuantity = 25, avgTime = 3.0 -> total = 75.0
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-010", "GaN Epitaxial 4-inch", 3.0, 0.4));
    sampleRepo.AddStock("S-010", 0);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-010", "Beta Inc", 10, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));

    ASSERT_FALSE(queue.Empty());
    EXPECT_DOUBLE_EQ(queue.Front().totalTimeMin, 75.0);
}

TEST(OrderApproval, MultipleInsufficientStockApprovalsEnqueueJobsInFifoOrder) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.9));
    sampleRepo.Register(MakeSample("S-002", "GaN Epitaxial 4-inch", 0.3, 0.8));

    OrderRepository orderRepo;
    Order firstOrder;
    Order secondOrder;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 100, firstOrder));
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-002", "Beta Inc", 50, secondOrder));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, firstOrder.orderId));
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, secondOrder.orderId));

    ASSERT_EQ(queue.Size(), 2u);
    EXPECT_EQ(queue.Front().orderId, firstOrder.orderId);
    queue.Dequeue();
    EXPECT_EQ(queue.Front().orderId, secondOrder.orderId);
}

TEST(OrderApproval, RejectTransitionsReservedToRejected) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, order));

    EXPECT_TRUE(orderRepo.Reject(order.orderId));
    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::REJECTED);
}

TEST(OrderApproval, ApprovingAlreadyRejectedOrderIsRefused) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    sampleRepo.AddStock("S-001", 100);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, order));
    ASSERT_TRUE(orderRepo.Reject(order.orderId));

    ProductionQueue queue;
    EXPECT_FALSE(orderRepo.Approve(sampleRepo, queue, order.orderId));
    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::REJECTED);
}

TEST(OrderApproval, RejectingAlreadyConfirmedOrderIsRefused) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    sampleRepo.AddStock("S-001", 100);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));
    ASSERT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::CONFIRMED);

    EXPECT_FALSE(orderRepo.Reject(order.orderId));
    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::CONFIRMED);
}

TEST(OrderApproval, ApprovingAlreadyConfirmedOrderIsRefused) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    sampleRepo.AddStock("S-001", 100);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));
    ASSERT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::CONFIRMED);

    EXPECT_FALSE(orderRepo.Approve(sampleRepo, queue, order.orderId));
    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::CONFIRMED);
}

TEST(OrderApproval, ApprovingAlreadyProducingOrderIsRefused) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-003", "SiC Power Substrate 6-inch", 2.0, 0.9));
    sampleRepo.AddStock("S-003", 30);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-003", "Samsung Foundry", 200, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));
    ASSERT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::PRODUCING);

    EXPECT_FALSE(orderRepo.Approve(sampleRepo, queue, order.orderId));
    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::PRODUCING);
}

TEST(OrderApproval, RejectingAlreadyRejectedOrderIsRefused) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 10, order));
    ASSERT_TRUE(orderRepo.Reject(order.orderId));
    ASSERT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::REJECTED);

    EXPECT_FALSE(orderRepo.Reject(order.orderId));
    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::REJECTED);
}

TEST(OrderApproval, RejectingAlreadyProducingOrderIsRefused) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-003", "SiC Power Substrate 6-inch", 2.0, 0.9));
    sampleRepo.AddStock("S-003", 30);

    OrderRepository orderRepo;
    Order order;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-003", "Samsung Foundry", 200, order));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, order.orderId));
    ASSERT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::PRODUCING);

    EXPECT_FALSE(orderRepo.Reject(order.orderId));
    EXPECT_EQ(FindOrderById(orderRepo, order.orderId).status, OrderStatus::PRODUCING);
}

TEST(OrderApproval, ApprovingNonexistentOrderIdReturnsFalse) {
    SampleRepository sampleRepo;
    OrderRepository orderRepo;
    ProductionQueue queue;

    EXPECT_FALSE(orderRepo.Approve(sampleRepo, queue, "ORD-NO-SUCH-ORDER"));
}

TEST(OrderApproval, RejectingNonexistentOrderIdReturnsFalse) {
    OrderRepository orderRepo;

    EXPECT_FALSE(orderRepo.Reject("ORD-NO-SUCH-ORDER"));
}

TEST(OrderApproval, ListingOrdersAwaitingApprovalReturnsOnlyReservedOrders) {
    SampleRepository sampleRepo;
    sampleRepo.Register(MakeSample("S-001", "Silicon Wafer 8-inch", 0.5, 0.92));
    sampleRepo.AddStock("S-001", 100);

    OrderRepository orderRepo;
    Order reservedOrder;
    Order toBeConfirmedOrder;
    Order toBeRejectedOrder;
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Acme Corp", 5, reservedOrder));
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Beta Inc", 10, toBeConfirmedOrder));
    ASSERT_TRUE(orderRepo.Register(sampleRepo, "S-001", "Gamma LLC", 15, toBeRejectedOrder));

    ProductionQueue queue;
    ASSERT_TRUE(orderRepo.Approve(sampleRepo, queue, toBeConfirmedOrder.orderId));
    ASSERT_TRUE(orderRepo.Reject(toBeRejectedOrder.orderId));

    const auto reservedOrders = orderRepo.FindByStatus(OrderStatus::RESERVED);

    ASSERT_EQ(reservedOrders.size(), 1u);
    EXPECT_EQ(reservedOrders[0].orderId, reservedOrder.orderId);
}
