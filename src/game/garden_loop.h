#pragma once

#include <string>
#include <vector>

#include "core/plant.h"
#include "core/rng.h"
#include "data/gardeners.h"
#include "game/garden.h"

class GardenLoop {
public:
    GardenLoop(const std::vector<PlantTemplate>& pool, const std::vector<Gardener>& gardeners);

    void rollShop(GardenState& g, Rng& rng, bool force);
    bool buy(GardenState& g, int shopIndex, std::string& msg, Rng& rng);
    bool playFromHand(GardenState& g, int handIndex, std::string& msg, Rng& rng,
                      int boardIndex = -1);
    bool sellFromBoard(GardenState& g, int boardIndex, std::string& msg);
    bool moveBoard(GardenState& g, int from, int to, std::string& msg);
    bool refresh(GardenState& g, Rng& rng, std::string& msg);
    void freeze(GardenState& g);
    bool upgradeTier(GardenState& g, std::string& msg);
    bool canUseSkill(const GardenState& g) const;
    bool useSkill(GardenState& g, Rng& rng, std::string& msg);
    bool pickDiscover(GardenState& g, int index, std::string& msg, Rng& rng);
    bool hasDiscover(const GardenState& g) const { return !g.discover.empty(); }

    const Gardener& gardener(int id) const;
    int bonusIncome(int gardenerId) const;
    int startingArmor(int gardenerId) const;
    int upgradeCost(int tier) const { return upgradeCostForTier(tier); }

private:
    bool blocked(const GardenState& g, std::string& msg) const;
    void resolveTriples(GardenState& g, std::string& msg, Rng& rng);
    void rollDiscover(GardenState& g, Rng& rng);
    void mergeGolden(GardenState& g, int templateId);

    const std::vector<PlantTemplate>* pool_;
    const std::vector<Gardener>* gardeners_;
    std::vector<int> eligible_[kMaxGardenTier + 1];
};
