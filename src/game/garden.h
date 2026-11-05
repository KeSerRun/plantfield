#pragma once

#include <string>
#include <vector>

#include "core/plant.h"

constexpr int kHandLimit = 10;
constexpr int kMaxGardenTier = 6;
constexpr int kMinionCost = 3;
constexpr int kSellRefund = 1;
constexpr int kRefreshCost = 1;
constexpr int kMaxSunlight = 10;
constexpr int kStartHealth = 40;

struct GardenState {
    std::string name;
    int sunlight = 3;
    int gardenTier = 1;
    int health = kStartHealth;
    int shield = 0;
    int gardenerId = 0;
    bool skillUsed = false;
    bool shopFrozen = false;
    std::vector<Plant> board;
    std::vector<Plant> hand;
    std::vector<Plant> shop;
    std::vector<Plant> discover;
};

inline int shopSizeForTier(int tier) {
    static const int table[kMaxGardenTier + 1] = {0, 3, 3, 4, 5, 6, 6};
    if (tier < 1) {
        tier = 1;
    }
    if (tier > kMaxGardenTier) {
        tier = kMaxGardenTier;
    }
    return table[tier];
}

inline int upgradeCostForTier(int tier) {
    static const int table[kMaxGardenTier + 1] = {0, 4, 5, 6, 7, 8, 0};
    if (tier < 1 || tier >= kMaxGardenTier) {
        return -1;
    }
    return table[tier];
}

inline int sunlightIncome(int round) {
    const int income = 3 + (round - 1);
    return income > kMaxSunlight ? kMaxSunlight : income;
}
