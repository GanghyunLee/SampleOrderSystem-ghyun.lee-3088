#pragma once

#include <string>
#include <vector>

#include "JsonValue.h"
#include "Order.h"
#include "SampleRepository.h"

class ProductionQueue;

// Holds registered Order entities and owns order-number issuance. See
// docs/FEATURES/03-order-registration.md for the registration rules this
// repository must enforce, and docs/FEATURES/04-order-approval-rejection.md
// for the approval/rejection state transitions.
class OrderRepository {
public:
    // Registers a new order (initial status RESERVED) for sampleId, after
    // validating against sampleRepo and the input rules below. On success,
    // fills outOrder with the generated Order (including the auto-issued
    // orderId in "ORD-YYYYMMDD-NNNN" format) and returns true.
    //
    // Rejects (returns false, does not store, does not consume an order
    // number) when:
    //  - sampleId does not exist in sampleRepo
    //  - quantity < 1
    //  - customerName is empty or contains only whitespace
    //
    // This method never inspects or mutates Sample.stock: stock
    // availability is checked later, at approval time (see
    // docs/FEATURES/04-order-approval-rejection.md), not here.
    bool Register(const SampleRepository& sampleRepo, const std::string& sampleId,
                  const std::string& customerName, int quantity, Order& outOrder);

    // Returns every registered order, in the order they were registered.
    std::vector<Order> FindAll() const;

    // Returns every registered order whose status equals the given status.
    std::vector<Order> FindByStatus(OrderStatus status) const;

    // Approves a RESERVED order: if sampleRepo's current stock for the
    // order's sample is sufficient (stock >= quantity), decreases that stock
    // by quantity and moves the order to CONFIRMED. Otherwise, consumes the
    // existing stock down to 0, enqueues a ProductionJob (shortage = quantity
    // - stock, actualQuantity = ceil(shortage / yield), totalTimeMin =
    // avgProductionTimeMin * actualQuantity) into queue, and moves the order
    // to PRODUCING. Returns false (no state change) when orderId does not
    // exist, the order is not RESERVED, or the sample no longer exists. See
    // docs/FEATURES/04-order-approval-rejection.md and
    // docs/architecture.md section 5.
    bool Approve(SampleRepository& sampleRepo, ProductionQueue& queue, const std::string& orderId);

    // Rejects a RESERVED order, moving it to REJECTED. Returns false (no
    // state change) when orderId does not exist or the order is not
    // RESERVED.
    bool Reject(const std::string& orderId);

    // Serializes all held orders to a JsonValue, and restores held orders
    // from a previously produced JsonValue (see docs/architecture.md
    // section 6 for the persistence round-trip requirement). FromJson
    // replaces whatever orders were previously held.
    JsonValue ToJson() const;
    void FromJson(const JsonValue& json);

private:
    std::vector<Order> orders_;
    int nextOrderSequence_ = 1;

    std::string IssueOrderId();
};
