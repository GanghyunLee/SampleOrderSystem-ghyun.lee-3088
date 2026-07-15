#pragma once

#include <string>
#include <vector>

#include "Sample.h"

// Holds registered Sample entities. See docs/FEATURES/02-sample-management.md
// for registration/search rules.
class SampleRepository {
public:
    // Registers a new sample. Rejects (returns false, does not store) when:
    //  - sample.id already exists in the repository
    //  - sample.yield is out of the (0, 1] range
    //  - sample.id or sample.name is empty
    // On success, the stored sample's stock starts at 0 regardless of any
    // stock value the caller may have set on the input struct.
    bool Register(const Sample& sample);

    // Returns every registered sample, including its current stock.
    std::vector<Sample> FindAll() const;

    // Returns samples whose name contains keyword as a substring.
    std::vector<Sample> SearchByName(const std::string& keyword) const;

    // Finds the sample with the given id. Returns false (outSample untouched)
    // when no such sample is registered.
    bool FindById(const std::string& id, Sample& outSample) const;

    // Increases/decreases stock for the sample with the given id. Returns
    // false when no such sample is registered. DecreaseStock never takes
    // stock below 0 (callers are expected to request at most the current
    // stock, per docs/FEATURES/04-order-approval-rejection.md).
    bool AddStock(const std::string& id, int quantity);
    bool DecreaseStock(const std::string& id, int quantity);

private:
    std::vector<Sample> samples_;
};
