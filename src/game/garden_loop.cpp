#include "game/garden_loop.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>

#include "core/plant.h"
#include "game/garden_effects.h"

GardenLoop::GardenLoop(const std::vector<PlantTemplate>& pool,
                       const std::vector<Gardener>& gardeners)
    : pool_(&pool), gardeners_(&gardeners) {
    for (int tier = 1; tier <= kMaxGardenTier; ++tier) {
        for (std::size_t i = 0; i < pool_->size(); ++i) {
            if ((*pool_)[i].token) {
                continue;
            }
            if ((*pool_)[i].tier <= tier) {
                eligible_[tier].push_back(static_cast<int>(i));
            }
        }
    }
}

const Gardener& GardenLoop::gardener(int id) const {
    if (id < 0 || id >= static_cast<int>(gardeners_->size())) {
        id = 0;
    }
    return (*gardeners_)[static_cast<std::size_t>(id)];
}

int GardenLoop::bonusIncome(int gardenerId) const {
    return gardener(gardenerId).skill.bonusIncome;
}

int GardenLoop::startingArmor(int gardenerId) const {
    return gardener(gardenerId).skill.startingArmor;
}

bool GardenLoop::blocked(const GardenState& g, std::string& msg) const {
    if (!g.discover.empty()) {
        msg = "先选择三连奖励";
        return true;
    }
    return false;
}

void GardenLoop::rollShop(GardenState& g, Rng& rng, bool force) {
    if (g.shopFrozen && !force) {
        g.shopFrozen = false;
        return;
    }
    g.shopFrozen = false;
    g.shop.clear();

    const std::vector<int>& candidates = eligible_[g.gardenTier];
    if (candidates.empty()) {
        return;
    }
    const int size = shopSizeForTier(g.gardenTier);
    for (int i = 0; i < size; ++i) {
        const int id = rng.choice(candidates);
        g.shop.push_back(makePlant((*pool_)[static_cast<std::size_t>(id)], id));
    }
}

bool GardenLoop::buy(GardenState& g, int shopIndex, std::string& msg, Rng& rng) {
    if (blocked(g, msg)) {
        return false;
    }
    if (shopIndex < 0 || shopIndex >= static_cast<int>(g.shop.size())) {
        return false;
    }
    if (g.sunlight < kMinionCost) {
        msg = "阳光不足";
        return false;
    }
    if (static_cast<int>(g.hand.size()) >= kHandLimit) {
        msg = "手牌已满";
        return false;
    }

    const Plant picked = g.shop[static_cast<std::size_t>(shopIndex)];
    g.shop.erase(g.shop.begin() + shopIndex);
    g.sunlight -= kMinionCost;
    msg = "种植 " + (*pool_)[static_cast<std::size_t>(picked.templateId)].name;
    g.hand.push_back(picked);
    resolveTriples(g, msg, rng);
    return true;
}

bool GardenLoop::playFromHand(GardenState& g, int handIndex, std::string& msg, Rng& rng,
                              int boardIndex) {
    if (blocked(g, msg)) {
        return false;
    }
    if (handIndex < 0 || handIndex >= static_cast<int>(g.hand.size())) {
        return false;
    }
    if (static_cast<int>(g.board.size()) >= kBoardLimit) {
        msg = "花园已满";
        return false;
    }

    Plant picked = g.hand[static_cast<std::size_t>(handIndex)];
    g.hand.erase(g.hand.begin() + handIndex);

    const GardenerSkill& skill = gardener(g.gardenerId).skill;
    picked.attack += skill.playAttack;
    picked.maxHealth += skill.playHealth;
    picked.health += skill.playHealth;

    int pos = static_cast<int>(g.board.size());
    if (boardIndex >= 0 && boardIndex < pos) {
        pos = boardIndex;
    }
    g.board.insert(g.board.begin() + pos, picked);

    msg = "上场 " + (*pool_)[static_cast<std::size_t>(picked.templateId)].name;
    if (skill.playAttack != 0 || skill.playHealth != 0) {
        msg += "（" + skill.name + " +" + std::to_string(skill.playAttack) + "/+" +
               std::to_string(skill.playHealth) + "）";
    }
    runGardenEffects(g, pos, (*pool_)[static_cast<std::size_t>(picked.templateId)].battlecry,
                     *pool_, rng, 0);
    return true;
}

bool GardenLoop::moveBoard(GardenState& g, int from, int to, std::string& msg) {
    if (blocked(g, msg)) {
        return false;
    }
    const int count = static_cast<int>(g.board.size());
    if (from < 0 || from >= count || to < 0 || to >= count || from == to) {
        return false;
    }

    const Plant moved = g.board[static_cast<std::size_t>(from)];
    g.board.erase(g.board.begin() + from);
    g.board.insert(g.board.begin() + to, moved);
    msg = "调整站位 " + (*pool_)[static_cast<std::size_t>(moved.templateId)].name;
    return true;
}

bool GardenLoop::sellFromBoard(GardenState& g, int boardIndex, std::string& msg) {
    if (blocked(g, msg)) {
        return false;
    }
    if (boardIndex < 0 || boardIndex >= static_cast<int>(g.board.size())) {
        return false;
    }

    const Plant picked = g.board[static_cast<std::size_t>(boardIndex)];
    msg = "铲除 " + (*pool_)[static_cast<std::size_t>(picked.templateId)].name;
    g.board.erase(g.board.begin() + boardIndex);
    g.sunlight += kSellRefund;
    return true;
}

bool GardenLoop::refresh(GardenState& g, Rng& rng, std::string& msg) {
    if (blocked(g, msg)) {
        return false;
    }
    if (g.sunlight < kRefreshCost) {
        msg = "阳光不足";
        return false;
    }
    g.sunlight -= kRefreshCost;
    rollShop(g, rng, true);
    msg = "浇水重抽商店";
    return true;
}

void GardenLoop::freeze(GardenState& g) {
    if (!g.discover.empty()) {
        return;
    }
    g.shopFrozen = !g.shopFrozen;
}

bool GardenLoop::upgradeTier(GardenState& g, std::string& msg) {
    if (blocked(g, msg)) {
        return false;
    }
    const int cost = upgradeCostForTier(g.gardenTier);
    if (cost < 0) {
        msg = "已是最高花园等级";
        return false;
    }
    if (g.sunlight < cost) {
        msg = "阳光不足";
        return false;
    }

    g.sunlight -= cost;
    ++g.gardenTier;
    msg = "升级草坪到 " + std::to_string(g.gardenTier) + " 级";
    return true;
}

bool GardenLoop::canUseSkill(const GardenState& g) const {
    const GardenerSkill& skill = gardener(g.gardenerId).skill;
    return skill.active && !g.skillUsed && g.sunlight >= skill.cost;
}

bool GardenLoop::useSkill(GardenState& g, Rng& rng, std::string& msg) {
    if (blocked(g, msg)) {
        return false;
    }
    const GardenerSkill& skill = gardener(g.gardenerId).skill;
    if (!skill.active) {
        msg = "这是被动技能";
        return false;
    }
    if (g.skillUsed) {
        msg = "本回合已使用过技能";
        return false;
    }
    if (g.sunlight < skill.cost) {
        msg = "阳光不足";
        return false;
    }

    g.sunlight -= skill.cost;
    g.skillUsed = true;
    runGardenEffects(g, -1, skill.effects, *pool_, rng, 0);
    msg = "使用技能「" + skill.name + "」";
    return true;
}

void GardenLoop::resolveTriples(GardenState& g, std::string& msg, Rng& rng) {
    if (!g.discover.empty()) {
        return;
    }
    for (bool merged = true; merged;) {
        merged = false;

        std::map<int, int> counts;
        for (const Plant& p : g.hand) {
            if (p.templateId >= 0 && p.goldenLevel == 0) {
                ++counts[p.templateId];
            }
        }
        for (const Plant& p : g.board) {
            if (p.templateId >= 0 && p.goldenLevel == 0) {
                ++counts[p.templateId];
            }
        }

        for (const auto& entry : counts) {
            if (entry.second >= 3) {
                mergeGolden(g, entry.first);
                msg += " ｜ 光合融合：" + (*pool_)[static_cast<std::size_t>(entry.first)].name +
                       " 升为金色";
                rollDiscover(g, rng);
                merged = true;
                break;
            }
        }
        if (!g.discover.empty()) {
            break;
        }
    }
}

void GardenLoop::rollDiscover(GardenState& g, Rng& rng) {
    g.discover.clear();

    // 三连奖励：只出"正好高一个草坪等级"的随从，而不是商店那种累积池
    int tier = std::min(g.gardenTier + 1, kMaxGardenTier);
    std::vector<int> candidates;
    while (tier >= 1) {
        candidates.clear();
        for (std::size_t i = 0; i < pool_->size(); ++i) {
            const PlantTemplate& t = (*pool_)[i];
            if (!t.token && t.tier == tier) {
                candidates.push_back(static_cast<int>(i));
            }
        }
        if (!candidates.empty()) {
            break;
        }
        --tier;
    }

    for (int i = 0; i < 3 && !candidates.empty(); ++i) {
        const int pick = rng.range(0, static_cast<int>(candidates.size()) - 1);
        const int templateId = candidates[static_cast<std::size_t>(pick)];
        g.discover.push_back(makePlant((*pool_)[static_cast<std::size_t>(templateId)], templateId));
        candidates.erase(candidates.begin() + pick);
    }
}

bool GardenLoop::pickDiscover(GardenState& g, int index, std::string& msg, Rng& rng) {
    if (index < 0 || index >= static_cast<int>(g.discover.size())) {
        return false;
    }
    const Plant picked = g.discover[static_cast<std::size_t>(index)];
    g.discover.clear();
    msg = "三连奖励：" + (*pool_)[static_cast<std::size_t>(picked.templateId)].name;
    g.hand.push_back(picked);
    resolveTriples(g, msg, rng);
    return true;
}

void GardenLoop::mergeGolden(GardenState& g, int templateId) {
    int remaining = 3;

    for (int i = static_cast<int>(g.hand.size()) - 1; i >= 0 && remaining > 0; --i) {
        if (g.hand[static_cast<std::size_t>(i)].templateId == templateId &&
            g.hand[static_cast<std::size_t>(i)].goldenLevel == 0) {
            g.hand.erase(g.hand.begin() + i);
            --remaining;
        }
    }
    for (int i = static_cast<int>(g.board.size()) - 1; i >= 0 && remaining > 0; --i) {
        if (g.board[static_cast<std::size_t>(i)].templateId == templateId &&
            g.board[static_cast<std::size_t>(i)].goldenLevel == 0) {
            g.board.erase(g.board.begin() + i);
            --remaining;
        }
    }

    Plant golden = makePlant((*pool_)[static_cast<std::size_t>(templateId)], templateId);
    golden.goldenLevel = 1;
    golden.attack *= 2;
    golden.health *= 2;
    golden.maxHealth *= 2;
    g.hand.push_back(golden);
}
