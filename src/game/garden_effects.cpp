#include "game/garden_effects.h"

#include <cstddef>
#include <vector>

namespace {

constexpr int kMaxTriggerDepth = 6;

}  // namespace

void runGardenEffects(GardenState& g, int casterIndex, const std::vector<Effect>& effects,
                      const std::vector<PlantTemplate>& pool, Rng& rng, int depth) {
    if (depth > kMaxTriggerDepth || effects.empty()) {
        return;
    }
    if (casterIndex >= static_cast<int>(g.board.size())) {
        casterIndex = -1;
    }

    for (const Effect& e : effects) {
        if (e.action == EffectAction::Summon) {
            for (int i = 0; i < e.summonCount; ++i) {
                if (static_cast<int>(g.board.size()) >= kBoardLimit) {
                    break;
                }
                Plant summoned =
                    makePlant(pool[static_cast<std::size_t>(e.summonTemplateId)], e.summonTemplateId);
                g.board.push_back(summoned);
                runGardenEffects(g, static_cast<int>(g.board.size()) - 1,
                                 pool[static_cast<std::size_t>(e.summonTemplateId)].battlecry, pool,
                                 rng, depth + 1);
            }
            continue;
        }

        std::vector<int> targets;
        const bool hasCaster = casterIndex >= 0;
        switch (e.target) {
        case TargetSelector::Self:
            if (hasCaster) {
                targets.push_back(casterIndex);
            }
            break;
        case TargetSelector::Adjacent:
            if (hasCaster) {
                if (casterIndex - 1 >= 0) {
                    targets.push_back(casterIndex - 1);
                }
                if (casterIndex + 1 < static_cast<int>(g.board.size())) {
                    targets.push_back(casterIndex + 1);
                }
            }
            break;
        case TargetSelector::AllFriendlies:
            for (int i = 0; i < static_cast<int>(g.board.size()); ++i) {
                targets.push_back(i);
            }
            break;
        case TargetSelector::AllFriendliesOther:
            for (int i = 0; i < static_cast<int>(g.board.size()); ++i) {
                if (i != casterIndex) {
                    targets.push_back(i);
                }
            }
            break;
        case TargetSelector::RandomFriendly:
            if (!g.board.empty()) {
                targets.push_back(rng.range(0, static_cast<int>(g.board.size()) - 1));
            }
            break;
        case TargetSelector::RandomFriendlyOther: {
            std::vector<int> friendly;
            for (int i = 0; i < static_cast<int>(g.board.size()); ++i) {
                if (i != casterIndex) {
                    friendly.push_back(i);
                }
            }
            if (!friendly.empty()) {
                targets.push_back(rng.choice(friendly));
            }
            break;
        }
        case TargetSelector::AllFriendliesSameRace:
        case TargetSelector::AllFriendliesSameRaceOther: {
            if (!hasCaster) {
                break;
            }
            const std::string& race =
                pool[static_cast<std::size_t>(g.board[static_cast<std::size_t>(casterIndex)].templateId)]
                    .race;
            for (int i = 0; i < static_cast<int>(g.board.size()); ++i) {
                if (i == casterIndex && e.target == TargetSelector::AllFriendliesSameRaceOther) {
                    continue;
                }
                if (pool[static_cast<std::size_t>(g.board[static_cast<std::size_t>(i)].templateId)].race !=
                    race) {
                    continue;
                }
                targets.push_back(i);
            }
            break;
        }
        default:
            break;
        }

        if (e.action != EffectAction::Buff) {
            continue;
        }
        for (int index : targets) {
            Plant& target = g.board[static_cast<std::size_t>(index)];
            target.attack += e.attackBonus;
            target.maxHealth += e.healthBonus;
            target.health += e.healthBonus;
        }
    }
}
