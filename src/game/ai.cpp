#include "game/ai.h"

#include <cstddef>
#include <string>
#include <vector>

namespace {

int score(const Plant& p) {
    return p.attack + p.health;
}

int bestShopIndex(const GardenState& g) {
    int best = -1;
    int bestScore = -1;
    for (std::size_t i = 0; i < g.shop.size(); ++i) {
        if (score(g.shop[i]) > bestScore) {
            bestScore = score(g.shop[i]);
            best = static_cast<int>(i);
        }
    }
    return best;
}

int weakestBoardIndex(const GardenState& g) {
    int worst = -1;
    int worstScore = 0;
    for (std::size_t i = 0; i < g.board.size(); ++i) {
        const int s = score(g.board[i]);
        if (worst < 0 || s < worstScore) {
            worst = static_cast<int>(i);
            worstScore = s;
        }
    }
    return worst;
}

int bestHandIndex(const GardenState& g) {
    int best = -1;
    int bestScore = -1;
    for (std::size_t i = 0; i < g.hand.size(); ++i) {
        if (score(g.hand[i]) > bestScore) {
            bestScore = score(g.hand[i]);
            best = static_cast<int>(i);
        }
    }
    return best;
}

}  // namespace

void AiAgent::takeTurn(GardenState& g, GardenLoop& loop, Rng& rng, int round,
                       std::vector<std::string>& actions) {
    std::string msg;

    const auto playAll = [&]() {
        while (static_cast<int>(g.board.size()) < kBoardLimit && !g.hand.empty()) {
            if (!loop.playFromHand(g, 0, msg, rng)) {
                break;
            }
            actions.push_back(msg);
        }
    };

    const auto rebalance = [&]() {
        while (static_cast<int>(g.board.size()) >= kBoardLimit && !g.hand.empty()) {
            const int weakest = weakestBoardIndex(g);
            const int best = bestHandIndex(g);
            if (weakest < 0 || best < 0) {
                break;
            }
            if (score(g.hand[static_cast<std::size_t>(best)]) <=
                score(g.board[static_cast<std::size_t>(weakest)])) {
                break;
            }
            if (!loop.sellFromBoard(g, weakest, msg)) {
                break;
            }
            actions.push_back(msg);
            if (!loop.playFromHand(g, best, msg, rng)) {
                break;
            }
            actions.push_back(msg);
        }
    };

    const auto resolveDiscover = [&]() {
        while (!g.discover.empty()) {
            int best = 0;
            for (int i = 1; i < static_cast<int>(g.discover.size()); ++i) {
                if (score(g.discover[static_cast<std::size_t>(i)]) >
                    score(g.discover[static_cast<std::size_t>(best)])) {
                    best = i;
                }
            }
            if (!loop.pickDiscover(g, best, msg, rng)) {
                break;
            }
            actions.push_back(msg);
        }
    };

    const auto buyWorthwhile = [&](int reserve) {
        for (bool again = true; again;) {
            again = false;
            if (g.sunlight - kMinionCost < reserve) {
                break;
            }
            if (static_cast<int>(g.hand.size()) >= kHandLimit) {
                break;
            }

            const int index = bestShopIndex(g);
            if (index < 0) {
                break;
            }

            if (static_cast<int>(g.board.size()) >= kBoardLimit) {
                const int weakest = weakestBoardIndex(g);
                if (weakest >= 0 && score(g.shop[static_cast<std::size_t>(index)]) <=
                                        score(g.board[static_cast<std::size_t>(weakest)])) {
                    break;
                }
            }

            if (loop.buy(g, index, msg, rng)) {
                actions.push_back(msg);
                resolveDiscover();
                again = true;
            }
        }
    };

    const int upgradeCost = (g.gardenTier < kMaxGardenTier) ? loop.upgradeCost(g.gardenTier) : -1;
    const bool tiering = upgradeCost >= 0 && round >= 2;

    resolveDiscover();

    // 场面铺满后开始为本轮升级攒钱（酒馆战棋里升级要牺牲一回合的经济）
    int reserve = 0;
    if (tiering && static_cast<int>(g.board.size()) >= kBoardLimit) {
        reserve = upgradeCost;
    }

    buyWorthwhile(reserve);
    playAll();
    rebalance();

    if (tiering && g.sunlight >= upgradeCost && loop.upgradeTier(g, msg)) {
        actions.push_back(msg);
    }

    for (int pass = 0; pass < 2; ++pass) {
        buyWorthwhile(0);
        playAll();
        rebalance();
        if (loop.canUseSkill(g) && loop.useSkill(g, rng, msg)) {
            actions.push_back(msg);
            playAll();
            rebalance();
        }
    }

    const int weakest = weakestBoardIndex(g);
    const int best = bestShopIndex(g);
    const bool dudShop =
        !g.shop.empty() && best >= 0 &&
        (weakest < 0 || score(g.shop[static_cast<std::size_t>(best)]) <=
                            score(g.board[static_cast<std::size_t>(weakest)]));

    if (dudShop && static_cast<int>(g.hand.size()) < kHandLimit &&
        g.sunlight >= kMinionCost + kRefreshCost) {
        if (loop.refresh(g, rng, msg)) {
            actions.push_back(msg);
            buyWorthwhile(0);
            playAll();
            rebalance();
        }
    }
}
