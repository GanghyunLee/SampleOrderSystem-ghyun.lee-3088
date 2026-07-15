#pragma once

#include <string>

#include "OrderStatus.h"

// Order domain entity. See docs/architecture.md section 3.2 and
// docs/FEATURES/03-order-registration.md for field definitions and rules.
struct Order {
    std::string orderId;        // e.g. "ORD-20260416-0043", auto-numbered
    std::string sampleId;       // must reference an existing Sample.id
    std::string customerName;   // must not be blank
    int quantity = 0;           // must be >= 1
    OrderStatus status = OrderStatus::RESERVED;
};
