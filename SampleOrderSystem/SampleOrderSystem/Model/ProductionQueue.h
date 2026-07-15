#pragma once

#include <deque>

#include "ProductionJob.h"

// FIFO production queue. See docs/architecture.md section 3.3.
class ProductionQueue {
public:
    void Enqueue(const ProductionJob& job);
    ProductionJob Front() const;
    void Dequeue();
    bool Empty() const;
    size_t Size() const;

private:
    std::deque<ProductionJob> jobs_;
};
