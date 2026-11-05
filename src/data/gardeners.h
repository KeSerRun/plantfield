#pragma once

#include <string>
#include <utility>
#include <vector>

#include "core/effect.h"
#include "data/templates.h"

struct GardenerSkill {
    std::string name;
    std::string description;
    bool active = false;
    int cost = 0;
    int bonusIncome = 0;
    int startingArmor = 0;
    int playAttack = 0;
    int playHealth = 0;
    std::vector<Effect> effects;
};

struct Gardener {
    std::string id;
    std::string name;
    GardenerSkill skill;
};

inline Gardener makeGardener(std::string id, std::string name, GardenerSkill skill) {
    Gardener g;
    g.id = std::move(id);
    g.name = std::move(name);
    g.skill = std::move(skill);
    return g;
}

inline GardenerSkill activeSkill(std::string name, std::string description, int cost,
                                 std::vector<Effect> effects) {
    GardenerSkill s;
    s.name = std::move(name);
    s.description = std::move(description);
    s.active = true;
    s.cost = cost;
    s.effects = std::move(effects);
    return s;
}

inline GardenerSkill passiveSkill(std::string name, std::string description) {
    GardenerSkill s;
    s.name = std::move(name);
    s.description = std::move(description);
    s.active = false;
    return s;
}

inline std::vector<Gardener> makeGardenerPool() {
    GardenerSkill sower = activeSkill("播种", "花 2 阳光：召唤 2 个幼苗", 2,
                                      {summonEffect(plant_ids::kSeedling, 2)});
    GardenerSkill horticulturist = activeSkill("修剪", "花 2 阳光：给随机友方植物 +3/+3", 2,
                                               {buffEffect(TargetSelector::RandomFriendly, 3, 3)});
    GardenerSkill propagator = activeSkill("催芽", "花 3 阳光：召唤 3 个幼苗", 3,
                                           {summonEffect(plant_ids::kSeedling, 3)});
    GardenerSkill mulcher = passiveSkill("护根", "开局获得 15 点护甲");
    mulcher.startingArmor = 15;
    GardenerSkill sunkeeper = passiveSkill("日照", "每回合额外获得 1 阳光");
    sunkeeper.bonusIncome = 1;
    GardenerSkill compostman = passiveSkill("堆肥", "你上场的植物额外 +1/+1");
    compostman.playAttack = 1;
    compostman.playHealth = 1;

    return {
        makeGardener("sower", "播种者", sower),
        makeGardener("horticulturist", "园艺师", horticulturist),
        makeGardener("propagator", "育苗师", propagator),
        makeGardener("mulcher", "护根园丁", mulcher),
        makeGardener("sunkeeper", "阳光园丁", sunkeeper),
        makeGardener("compostman", "肥料商", compostman),
    };
}
