#include "SampleRepository.h"

#include <algorithm>

bool SampleRepository::Register(const Sample& sample) {
    if (sample.id.empty() || sample.name.empty()) {
        return false;
    }
    if (sample.yield <= 0.0 || sample.yield > 1.0) {
        return false;
    }

    const bool idExists = std::any_of(samples_.begin(), samples_.end(),
        [&sample](const Sample& existing) { return existing.id == sample.id; });
    if (idExists) {
        return false;
    }

    Sample stored = sample;
    stored.stock = 0;
    samples_.push_back(stored);
    return true;
}

std::vector<Sample> SampleRepository::FindAll() const {
    return samples_;
}

std::vector<Sample> SampleRepository::SearchByName(const std::string& keyword) const {
    std::vector<Sample> results;
    for (const auto& sample : samples_) {
        if (sample.name.find(keyword) != std::string::npos) {
            results.push_back(sample);
        }
    }
    return results;
}

bool SampleRepository::FindById(const std::string& id, Sample& outSample) const {
    const auto it = std::find_if(samples_.begin(), samples_.end(),
        [&id](const Sample& sample) { return sample.id == id; });
    if (it == samples_.end()) {
        return false;
    }
    outSample = *it;
    return true;
}

bool SampleRepository::AddStock(const std::string& id, int quantity) {
    const auto it = std::find_if(samples_.begin(), samples_.end(),
        [&id](const Sample& sample) { return sample.id == id; });
    if (it == samples_.end()) {
        return false;
    }
    it->stock += quantity;
    return true;
}

bool SampleRepository::DecreaseStock(const std::string& id, int quantity) {
    const auto it = std::find_if(samples_.begin(), samples_.end(),
        [&id](const Sample& sample) { return sample.id == id; });
    if (it == samples_.end()) {
        return false;
    }
    it->stock = (quantity < it->stock) ? it->stock - quantity : 0;
    return true;
}
