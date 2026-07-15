#include "ProductionQueue.h"

void ProductionQueue::Enqueue(const ProductionJob& job) {
    jobs_.push_back(job);
}

ProductionJob ProductionQueue::Front() const {
    return jobs_.front();
}

void ProductionQueue::Dequeue() {
    jobs_.pop_front();
}

bool ProductionQueue::Empty() const {
    return jobs_.empty();
}

size_t ProductionQueue::Size() const {
    return jobs_.size();
}
