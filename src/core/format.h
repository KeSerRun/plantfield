#pragma once

#include <algorithm>
#include <string>

#include "core/plant.h"

inline std::string formatStats(int attack, int health) {
    return "(" + std::to_string(attack) + "/" + std::to_string(std::max(health, 0)) + ")";
}

inline std::string plantStats(const Plant& p) {
    return formatStats(p.attack, p.health);
}
