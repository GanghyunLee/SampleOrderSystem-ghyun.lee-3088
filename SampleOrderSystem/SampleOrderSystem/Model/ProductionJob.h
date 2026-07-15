#pragma once

#include <string>

// Production queue item. See docs/architecture.md section 3.3 and
// docs/FEATURES/04-order-approval-rejection.md for how these fields are
// computed at order-approval time.
struct ProductionJob {
    std::string orderId;     // target order
    std::string sampleId;
    int shortage = 0;        // shortage = order quantity - stock at approval time
    int actualQuantity = 0;  // actual production quantity = ceil(shortage / yield)
    double totalTimeMin = 0; // total production time = avgProductionTimeMin * actualQuantity
    double progress = 0;     // 0.0 ~ 1.0, for console display
};
