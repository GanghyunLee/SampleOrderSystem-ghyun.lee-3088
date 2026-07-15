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
