#include "game/match.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "core/aura.h"
#include "core/format.h"

namespace {

constexpr int kMaxRounds = 40;

void applyHeroDamage(GardenState& g, int amount) {
    if (g.shield > 0) {
        const int absorbed = std::min(g.shield, amount);
        g.shield -= absorbed;
        amount -= absorbed;
    }
    g.health -= amount;
}

}  // namespace

Match::Match(const std::vector<PlantTemplate>& pool,
             const std::vector<Gardener>& gardeners,
             std::uint32_t seed,
             int humanGardener,
             bool verboseCombat)
    : pool_(&pool),
      gardeners_(&gardeners),
      verboseCombat_(verboseCombat),
      loop_(pool, gardeners),
      rng_(seed) {
    players_[0].name = "你的花园";
    players_[1].name = "对手花园";

    const int count = static_cast<int>(gardeners.size());
    if (count > 0) {
        players_[0].gardenerId = ((humanGardener % count) + count) % count;
        players_[1].gardenerId = (players_[0].gardenerId + 3) % count;
    }
    for (int i = 0; i < 2; ++i) {
        players_[i].shield = loop_.startingArmor(players_[i].gardenerId);
    }

    log("=== 花园对战开始 ===");
    for (int i = 0; i < 2; ++i) {
        const Gardener& gd = gardenerOf(i);
        log("[" + players_[i].name + "] 园丁：" + gd.name + " · " + gd.skill.name + "——" +
            gd.skill.description);
        if (players_[i].shield > 0) {
            log("  开局护甲 +" + std::to_string(players_[i].shield));
        }
    }
}

void Match::log(const std::string& line) {
    logLines_.push_back(line);
    if (!silent_) {
        std::cout << line << '\n';
    }
}

std::string Match::shopText(const GardenState& g) const {
    if (g.shop.empty()) {
        return "（空）";
    }
    std::string out;
    for (const Plant& p : g.shop) {
        if (!out.empty()) {
            out += "  ";
        }
        out += (*pool_)[static_cast<std::size_t>(p.templateId)].name;
        out += plantStats(p);
    }
    return out;
}

std::string Match::boardText(const GardenState& g) const {
    if (g.board.empty()) {
        return "（空）";
    }
    std::string out;
    for (int i = 0; i < static_cast<int>(g.board.size()); ++i) {
        const Plant& p = g.board[static_cast<std::size_t>(i)];
        if (!out.empty()) {
            out += "  ";
        }
        out += (*pool_)[static_cast<std::size_t>(p.templateId)].name;
        if (p.goldenLevel > 0) {
            out += "★";
        }
        out += formatStats(effectiveAttack(g.board, *pool_, i), effectiveHealth(g.board, *pool_, i));
    }
    return out;
}

bool Match::finished() const {
    return players_[0].health <= 0 || players_[1].health <= 0 || round_ > kMaxRounds;
}

void Match::startRound() {
    if (scene_ == MatchScene::Recruit) {
        return;
    }
    beginRoundInternal();
}

void Match::beginRoundInternal() {
    if (finished()) {
        scene_ = MatchScene::Finished;
        return;
    }
    scene_ = MatchScene::Recruit;

    log("");
    log("--- 第 " + std::to_string(round_) + " 回合 ---");

    const int income = sunlightIncome(round_);
    for (int i = 0; i < 2; ++i) {
        players_[i].sunlight =
            income + loop_.bonusIncome(players_[i].gardenerId);
        players_[i].skillUsed = false;
        loop_.rollShop(players_[i], rng_, false);
    }

    for (int i = 0; i < 2; ++i) {
        const GardenState& g = players_[i];
        log("[" + g.name + "] 草坪" + std::to_string(g.gardenTier) + " 级  阳光" +
            std::to_string(g.sunlight) + "  血" + std::to_string(g.health));
        if (i == 0) {
            log("  商店：" + shopText(g));
        }
    }
}

void Match::runAiTurn(int side) {
    GardenState& g = players_[side];
    std::vector<std::string> actions;
    ai_.takeTurn(g, loop_, rng_, round_, actions);
    for (const std::string& action : actions) {
        log("  行动：" + action);
    }
    log("  上场：" + boardText(g));
}

bool Match::playerAct(PlayerAction action, int index, int index2) {
    if (scene_ != MatchScene::Recruit) {
        return false;
    }

    GardenState& g = players_[0];
    std::string msg;
    bool ok = false;

    switch (action) {
    case PlayerAction::Buy:
        ok = loop_.buy(g, index, msg, rng_);
        break;
    case PlayerAction::Play:
        ok = loop_.playFromHand(g, index, msg, rng_, index2);
        break;
    case PlayerAction::Sell:
        ok = loop_.sellFromBoard(g, index, msg);
        break;
    case PlayerAction::Refresh:
        ok = loop_.refresh(g, rng_, msg);
        break;
    case PlayerAction::Upgrade:
        ok = loop_.upgradeTier(g, msg);
        break;
    case PlayerAction::Freeze:
        loop_.freeze(g);
        msg = g.shopFrozen ? "保鲜：商店已锁定" : "保鲜：已解锁";
        ok = true;
        break;
    case PlayerAction::Skill:
        ok = loop_.useSkill(g, rng_, msg);
        break;
    case PlayerAction::PickDiscover:
        ok = loop_.pickDiscover(g, index, msg, rng_);
        break;
    case PlayerAction::Move:
        ok = loop_.moveBoard(g, index, index2, msg);
        break;
    }

    if (ok) {
        log("  你：" + msg);
    }
    return ok;
}

void Match::beginCombat() {
    if (scene_ != MatchScene::Recruit) {
        return;
    }
    if (!players_[0].discover.empty()) {
        log("  三连奖励还没选，先选完再出战");
        return;
    }

    runAiTurn(1);

    const CombatSide left{players_[0].name, players_[0].board};
    const CombatSide right{players_[1].name, players_[1].board};

    CombatResolver resolver;
    const std::uint32_t seed = static_cast<std::uint32_t>(rng_.range(1, 1000000));
    frames_.clear();
    combatResult_ = resolver.resolve(left, right, seed, *pool_, verboseCombat_, &frames_);
    scene_ = MatchScene::Combat;
}

void Match::applyCombatResult() {
    if (combatResult_.winner == CombatResult::Winner::Draw) {
        log("[战斗] 平局，双方各受到自己花园等级的伤害");
        applyHeroDamage(players_[0], players_[0].gardenTier);
        applyHeroDamage(players_[1], players_[1].gardenTier);
    } else {
        const int winnerIndex = combatResult_.winner == CombatResult::Winner::Left ? 0 : 1;
        const int loserIndex = 1 - winnerIndex;
        const int survivors =
            winnerIndex == 0 ? combatResult_.leftSurvivors : combatResult_.rightSurvivors;
        const int damage = players_[winnerIndex].gardenTier + survivors;

        log("[战斗] " + players_[winnerIndex].name + " 获胜，存活 " + std::to_string(survivors) +
            " 株，对 " + players_[loserIndex].name + " 造成 " + std::to_string(damage) + " 点伤害");
        applyHeroDamage(players_[loserIndex], damage);
    }

    log("[血量] " + players_[0].name + " " + std::to_string(std::max(players_[0].health, 0)) +
        "  |  " + players_[1].name + " " + std::to_string(std::max(players_[1].health, 0)));
}

void Match::finishCombat() {
    if (scene_ != MatchScene::Combat) {
        return;
    }

    for (const CombatFrame& frame : frames_) {
        logLines_.push_back(frame.line);
    }
    frames_.clear();

    applyCombatResult();
    ++round_;
    beginRoundInternal();
}

void Match::autoRound() {
    if (finished()) {
        return;
    }
    startRound();
    runAiTurn(0);
    beginCombat();
    finishCombat();
}

void Match::run() {
    while (!finished()) {
        autoRound();
    }
    log("");
    log("=== " + verdictText() + " ===");
}

std::string Match::verdictText() const {
    if (players_[0].health <= 0 && players_[1].health <= 0) {
        return "双方同归于尽，平局";
    }
    if (players_[1].health <= 0) {
        return players_[0].name + " 获胜！";
    }
    if (players_[0].health <= 0) {
        return players_[1].name + " 获胜！";
    }
    const int winner = players_[0].health >= players_[1].health ? 0 : 1;
    return "达到 " + std::to_string(kMaxRounds) + " 回合上限，" + players_[winner].name +
           " 以血量优势获胜";
}
