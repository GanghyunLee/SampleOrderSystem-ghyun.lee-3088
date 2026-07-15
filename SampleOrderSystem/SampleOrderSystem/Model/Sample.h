#pragma once

#include <string>

// Sample domain entity. See docs/architecture.md section 3.1 and
// docs/FEATURES/02-sample-management.md for field definitions and rules.
struct Sample {
    std::string id;                     // e.g. "S-001"
    std::string name;
    double avgProductionTimeMin = 0.0;  // minutes per unit
    double yield = 0.0;                 // 0 < yield <= 1
    int stock = 0;                      // current stock, not a registration input
};
