#pragma once

#include <cstddef>
#include <vector>

#include "core/plant.h"

inline bool auraSourceAlive(const Plant& p) {
    return !p.dead && !p.destroyed && p.health > 0;
}

inline bool sameRace(const std::vector<PlantTemplate>& pool, const Plant& a, const Plant& b) {
    return pool[static_cast<std::size_t>(a.templateId)].race ==
           pool[static_cast<std::size_t>(b.templateId)].race;
}

inline bool auraApplies(TargetSelector selector, const std::vector<PlantTemplate>& pool,
                        const Plant& source, const Plant& target) {
    const bool self = source.instanceId == target.instanceId;
    switch (selector) {
    case TargetSelector::Self:
        return self;
    case TargetSelector::AllFriendlies:
        return true;
    case TargetSelector::AllFriendliesOther:
        return !self;
    case TargetSelector::AllFriendliesSameRace:
        return sameRace(pool, source, target);
    case TargetSelector::AllFriendliesSameRaceOther:
        return !self && sameRace(pool, source, target);
    default:
        return false;
    }
}

inline int auraAttackBonus(const std::vector<Plant>& board,
                           const std::vector<PlantTemplate>& pool,
                           int index) {
    const Plant& target = board[static_cast<std::size_t>(index)];
    int bonus = 0;
    for (const Plant& source : board) {
        if (!auraSourceAlive(source)) {
            continue;
        }
        const int scale = source.goldenLevel > 0 ? 2 : 1;
        for (const Effect& e : pool[static_cast<std::size_t>(source.templateId)].aura) {
            if (auraApplies(e.target, pool, source, target)) {
                bonus += e.attackBonus * scale;
            }
        }
    }
    return bonus;
}

inline int auraHealthBonus(const std::vector<Plant>& board,
                           const std::vector<PlantTemplate>& pool,
                           int index) {
    const Plant& target = board[static_cast<std::size_t>(index)];
    int bonus = 0;
    for (const Plant& source : board) {
        if (!auraSourceAlive(source)) {
            continue;
        }
        const int scale = source.goldenLevel > 0 ? 2 : 1;
        for (const Effect& e : pool[static_cast<std::size_t>(source.templateId)].aura) {
            if (auraApplies(e.target, pool, source, target)) {
                bonus += e.healthBonus * scale;
            }
        }
    }
    return bonus;
}

inline int effectiveAttack(const std::vector<Plant>& board,
                           const std::vector<PlantTemplate>& pool,
                           int index) {
    return board[static_cast<std::size_t>(index)].attack + auraAttackBonus(board, pool, index);
}

inline int effectiveHealth(const std::vector<Plant>& board,
                           const std::vector<PlantTemplate>& pool,
                           int index) {
    return board[static_cast<std::size_t>(index)].health + auraHealthBonus(board, pool, index);
}
