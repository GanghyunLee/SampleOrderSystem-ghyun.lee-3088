#include "OrderRepository.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "ProductionJob.h"
#include "ProductionQueue.h"

namespace {

bool IsBlank(const std::string& text) {
    return std::all_of(text.begin(), text.end(),
        [](unsigned char c) { return std::isspace(c) != 0; });
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

std::string OrderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::RESERVED: return "RESERVED";
        case OrderStatus::REJECTED: return "REJECTED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::RELEASE: return "RELEASE";
    }
    return "RESERVED";
}

OrderStatus OrderStatusFromString(const std::string& text) {
    if (text == "REJECTED") return OrderStatus::REJECTED;
    if (text == "PRODUCING") return OrderStatus::PRODUCING;
    if (text == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (text == "RELEASE") return OrderStatus::RELEASE;
    return OrderStatus::RESERVED;
}

} // namespace

std::string OrderRepository::IssueOrderId() {
    char sequenceBuf[8] = {0};
    std::snprintf(sequenceBuf, sizeof(sequenceBuf), "%04d", nextOrderSequence_);
    ++nextOrderSequence_;
    return "ORD-" + TodayAsYyyymmdd() + "-" + sequenceBuf;
}

bool OrderRepository::Register(const SampleRepository& sampleRepo, const std::string& sampleId,
                                const std::string& customerName, int quantity, Order& outOrder) {
    if (quantity < 1) {
        return false;
    }
    if (customerName.empty() || IsBlank(customerName)) {
        return false;
    }

    const auto samples = sampleRepo.FindAll();
    const bool sampleExists = std::any_of(samples.begin(), samples.end(),
        [&sampleId](const Sample& sample) { return sample.id == sampleId; });
    if (!sampleExists) {
        return false;
    }

    Order order;
    order.orderId = IssueOrderId();
    order.sampleId = sampleId;
    order.customerName = customerName;
    order.quantity = quantity;
    order.status = OrderStatus::RESERVED;

    orders_.push_back(order);
    outOrder = order;
    return true;
}

std::vector<Order> OrderRepository::FindAll() const {
    return orders_;
}

std::vector<Order> OrderRepository::FindByStatus(OrderStatus status) const {
    std::vector<Order> results;
    for (const auto& order : orders_) {
        if (order.status == status) {
            results.push_back(order);
        }
    }
    return results;
}

bool OrderRepository::Approve(SampleRepository& sampleRepo, ProductionQueue& queue,
                               const std::string& orderId) {
    const auto it = std::find_if(orders_.begin(), orders_.end(),
        [&orderId](const Order& order) { return order.orderId == orderId; });
    if (it == orders_.end() || it->status != OrderStatus::RESERVED) {
        return false;
    }

    Sample sample;
    if (!sampleRepo.FindById(it->sampleId, sample)) {
        return false;
    }

    if (sample.stock >= it->quantity) {
        sampleRepo.DecreaseStock(it->sampleId, it->quantity);
        it->status = OrderStatus::CONFIRMED;
        return true;
    }

    const int shortage = it->quantity - sample.stock;
    sampleRepo.DecreaseStock(it->sampleId, sample.stock);

    ProductionJob job;
    job.orderId = it->orderId;
    job.sampleId = it->sampleId;
    job.shortage = shortage;
    job.actualQuantity = static_cast<int>(std::ceil(shortage / sample.yield));
    job.totalTimeMin = sample.avgProductionTimeMin * job.actualQuantity;
    queue.Enqueue(job);

    it->status = OrderStatus::PRODUCING;
    return true;
}

bool OrderRepository::Reject(const std::string& orderId) {
    const auto it = std::find_if(orders_.begin(), orders_.end(),
        [&orderId](const Order& order) { return order.orderId == orderId; });
    if (it == orders_.end() || it->status != OrderStatus::RESERVED) {
        return false;
    }

    it->status = OrderStatus::REJECTED;
    return true;
}

JsonValue OrderRepository::ToJson() const {
    JsonValue root(std::map<std::string, JsonValue>{});
    JsonValue ordersJson(std::vector<JsonValue>{});

    for (const auto& order : orders_) {
        JsonValue orderJson(std::map<std::string, JsonValue>{});
        orderJson.Set("orderId", JsonValue(order.orderId));
        orderJson.Set("sampleId", JsonValue(order.sampleId));
        orderJson.Set("customerName", JsonValue(order.customerName));
        orderJson.Set("quantity", JsonValue(static_cast<double>(order.quantity)));
        orderJson.Set("status", JsonValue(OrderStatusToString(order.status)));
        ordersJson.Add(orderJson);
    }

    root.Set("orders", ordersJson);
    return root;
}

void OrderRepository::FromJson(const JsonValue& json) {
    orders_.clear();
    nextOrderSequence_ = 1;

    if (!json.Has("orders")) {
        return;
    }

    const std::string today = TodayAsYyyymmdd();
    const std::string todayPrefix = "ORD-" + today + "-";

    for (const auto& orderJson : json.Get("orders").AsArray()) {
        Order order;
        order.orderId = orderJson.Get("orderId").AsString();
        order.sampleId = orderJson.Get("sampleId").AsString();
        order.customerName = orderJson.Get("customerName").AsString();
        order.quantity = static_cast<int>(orderJson.Get("quantity").AsNumber());
        order.status = OrderStatusFromString(orderJson.Get("status").AsString());
        orders_.push_back(order);

        if (order.orderId.rfind(todayPrefix, 0) == 0) {
            const int sequence = std::atoi(order.orderId.substr(todayPrefix.size()).c_str());
            if (sequence >= nextOrderSequence_) {
                nextOrderSequence_ = sequence + 1;
            }
        }
    }
}
