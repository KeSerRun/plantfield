#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/combat.h"
#include "core/plant.h"
#include "core/rng.h"
#include "data/gardeners.h"
#include "game/ai.h"
#include "game/garden.h"
#include "game/garden_loop.h"

enum class PlayerAction {
    Buy,
    Play,
    Sell,
    Refresh,
    Upgrade,
    Freeze,
    Skill,
    PickDiscover,
    Move,
};

enum class MatchScene {
    None,
    Recruit,
    Combat,
    Finished,
};

class Match {
public:
    Match(const std::vector<PlantTemplate>& pool,
          const std::vector<Gardener>& gardeners,
          std::uint32_t seed,
          int humanGardener = 0,
          bool verboseCombat = false);

    void run();
    void autoRound();
    void startRound();
    bool playerAct(PlayerAction action, int index, int index2 = -1);
    void beginCombat();
    void finishCombat();

    MatchScene scene() const { return scene_; }
    bool awaitingPlayer() const { return scene_ == MatchScene::Recruit; }
    bool finished() const;
    void setSilent(bool silent) { silent_ = silent; }

    const Gardener& gardenerOf(int side) const { return loop_.gardener(players_[side].gardenerId); }
    const std::vector<CombatFrame>& combatFrames() const { return frames_; }
    std::string verdictText() const;
    int round() const { return round_; }
    int playerCount() const { return 2; }
    int humanIndex() const { return 0; }
    const GardenState& player(int index) const { return players_[index]; }
    const std::vector<PlantTemplate>& pool() const { return *pool_; }
    const std::vector<std::string>& logLines() const { return logLines_; }

private:
    void log(const std::string& line);
    void beginRoundInternal();
    void runAiTurn(int side);
    void applyCombatResult();
    std::string shopText(const GardenState& g) const;
    std::string boardText(const GardenState& g) const;

    const std::vector<PlantTemplate>* pool_;
    const std::vector<Gardener>* gardeners_;
    bool verboseCombat_ = false;
    bool silent_ = false;
    GardenLoop loop_;
    AiAgent ai_;
    Rng rng_;
    GardenState players_[2];
    int round_ = 1;
    MatchScene scene_ = MatchScene::None;
    std::vector<CombatFrame> frames_;
    CombatResult combatResult_;
    std::vector<std::string> logLines_;
};
