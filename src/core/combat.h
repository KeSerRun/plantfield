#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/effect.h"
#include "core/event_queue.h"
#include "core/plant.h"
#include "core/rng.h"

struct CombatSide {
    std::string name;
    std::vector<Plant> board;
};

struct CombatResult {
    enum class Winner { Left, Right, Draw };
    Winner winner = Winner::Draw;
    int leftSurvivors = 0;
    int rightSurvivors = 0;
};

struct CombatFrame {
    std::vector<Plant> left;
    std::vector<Plant> right;
    std::string line;
    int sourceId = -1;
    int targetId = -1;
    EventType type = EventType::Damage;
};

class CombatResolver {
public:
    CombatResult resolve(const CombatSide& left,
                         const CombatSide& right,
                         std::uint32_t seed,
                         const std::vector<PlantTemplate>& templates,
                         bool verbose = true,
                         std::vector<CombatFrame>* frames = nullptr);

private:
    struct Loc {
        int side = -1;
        int index = -1;
    };

    const PlantTemplate& templateOf(const Plant& p) const;
    Loc locate(int instanceId) const;
    const Plant* find(int instanceId) const;

    bool alive(int side, int index) const;
    bool isDeadAt(int side, int index) const;
    int effectiveAttackAt(int side, int index) const;
    int effectiveHealthAt(int side, int index) const;
    int aliveCount(int side) const;

    std::string label(int side, int index) const;
    std::string roster(int side) const;
    void log(const std::string& line);

    void assignInstanceIds();
    int nextInstanceId();

    std::vector<int> selectTargets(const Effect& e, int casterSide, int casterId);
    void runEffects(const std::vector<Effect>& effects, int goldenLevel, int casterSide,
                    int casterId);

    void markDead(int side, int index);
    void handleDamage(const GameEvent& e);
    void handleBuff(const GameEvent& e);
    void handleSummon(const GameEvent& e);
    void handleTrigger(const GameEvent& e, bool battlecry);
    void handleDeath(const GameEvent& e);
    void handleReborn(const GameEvent& e);
    void drainEvents();
    void removeDead();

    int takeAttacker(int side);
    int pickTarget(int attackerSide);
    void attackStep(int side);

    CombatSide sides_[2];
    std::vector<int> order_[2];
    std::size_t cursor_[2] = {0, 0};
    int nextInstanceId_ = 0;
    const std::vector<PlantTemplate>* templates_ = nullptr;
    bool verbose_ = true;
    std::vector<CombatFrame>* frames_ = nullptr;
    int currentSource_ = -1;
    int currentTarget_ = -1;
    EventType currentType_ = EventType::Damage;
    Rng rng_;
    EventQueue queue_;
};
