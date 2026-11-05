#pragma once

#include <string>
#include <utility>
#include <vector>

#include "core/effect.h"
#include "core/plant.h"

namespace plant_ids {
constexpr int kSeedling = 33;
}

inline PlantTemplate plantTemplate(std::string id, std::string name, int tier,
                                   int attack, int health, std::string race,
                                   std::vector<Keyword> keywords) {
    PlantTemplate t;
    t.id = std::move(id);
    t.name = std::move(name);
    t.tier = tier;
    t.attack = attack;
    t.health = health;
    t.race = std::move(race);
    t.keywords = std::move(keywords);
    return t;
}

inline PlantTemplate withBattlecry(PlantTemplate t, std::vector<Effect> effects) {
    t.battlecry = std::move(effects);
    return t;
}

inline PlantTemplate withDeathrattle(PlantTemplate t, std::vector<Effect> effects) {
    t.deathrattle = std::move(effects);
    return t;
}

inline PlantTemplate withAura(PlantTemplate t, std::vector<Effect> effects) {
    t.aura = std::move(effects);
    return t;
}

inline PlantTemplate asToken(PlantTemplate t) {
    t.token = true;
    return t;
}

inline std::vector<PlantTemplate> makePlantPool() {
    return {
        // ---- 1 级 ----
        withBattlecry(plantTemplate("sunflower", "向日葵", 1, 1, 3, "flower", {}),
                      {buffEffect(TargetSelector::RandomFriendlyOther, 1, 1)}),
        plantTemplate("cactus", "仙人掌", 1, 2, 3, "thorn", {Keyword::Thorns, Keyword::Reborn}),
        withDeathrattle(plantTemplate("rose", "玫瑰", 1, 3, 2, "flower", {}),
                        {damageEffect(TargetSelector::RandomEnemy, 1)}),
        plantTemplate("mushroom", "蘑菇", 1, 1, 5, "fungus", {Keyword::Shield}),
        plantTemplate("pea", "豌豆", 1, 2, 3, "vine", {Keyword::FastGrow}),
        plantTemplate("toxic_ivy", "毒藤", 1, 1, 2, "vine", {Keyword::Poison}),

        // ---- 2 级 ----
        plantTemplate("venus_flytrap", "捕蝇草", 2, 3, 4, "carnivorous", {Keyword::Poison}),
        withAura(plantTemplate("vine_whip", "藤鞭", 2, 2, 6, "vine", {Keyword::Thorns}),
                 {buffEffect(TargetSelector::AllFriendliesSameRaceOther, 0, 1)}),
        withDeathrattle(plantTemplate("dandelion", "蒲公英", 2, 2, 4, "flower", {Keyword::Shield}),
                        {summonEffect(plant_ids::kSeedling, 2)}),
        plantTemplate("bramble", "荆棘丛", 2, 4, 3, "thorn", {Keyword::Thorns, Keyword::Reborn}),
        withBattlecry(plantTemplate("pond_lily", "睡莲", 2, 2, 5, "aquatic", {Keyword::Shield}),
                      {buffEffect(TargetSelector::RandomFriendly, 1, 2)}),
        withDeathrattle(plantTemplate("sporeling", "孢芽", 2, 2, 2, "fungus", {}),
                        {summonEffect(plant_ids::kSeedling, 1)}),

        // ---- 3 级 ----
        withDeathrattle(plantTemplate("nightshade", "夜影", 3, 4, 5, "fungus", {Keyword::Poison}),
                        {damageEffect(TargetSelector::AllEnemies, 2)}),
        withBattlecry(plantTemplate("lotus", "莲花", 3, 3, 6, "flower", {Keyword::Shield}),
                      {buffEffect(TargetSelector::AllFriendliesOther, 1, 2)}),
        withAura(plantTemplate("giant_fern", "巨型蕨", 3, 6, 6, "vine", {Keyword::FastGrow}),
                 {buffEffect(TargetSelector::AllFriendliesSameRaceOther, 1, 0)}),
        withDeathrattle(plantTemplate("oak", "橡树", 3, 4, 8, "tree", {Keyword::Thorns}),
                        {buffEffect(TargetSelector::Adjacent, 2, 2)}),
        withAura(plantTemplate("coral", "珊瑚", 3, 3, 7, "aquatic", {Keyword::Thorns}),
                 {buffEffect(TargetSelector::AllFriendliesOther, 0, 1)}),
        plantTemplate("bramble_hulk", "荆魔", 3, 5, 5, "thorn", {Keyword::Poison}),

        // ---- 4 级 ----
        withDeathrattle(plantTemplate("elder_tree", "古树", 4, 6, 9, "tree", {Keyword::Thorns}),
                        {summonEffect(plant_ids::kSeedling, 3)}),
        withAura(plantTemplate("pitcher", "猪笼草", 4, 5, 6, "carnivorous", {Keyword::Poison}),
                 {buffEffect(TargetSelector::AllFriendliesSameRaceOther, 1, 1)}),
        withAura(plantTemplate("glow_moss", "荧光苔", 4, 4, 8, "fungus", {Keyword::Shield}),
                 {buffEffect(TargetSelector::AllFriendliesOther, 1, 2)}),
        withDeathrattle(plantTemplate("mangrove", "红树", 4, 5, 10, "aquatic", {Keyword::Thorns}),
                        {summonEffect(plant_ids::kSeedling, 2)}),
        withAura(plantTemplate("bloom_lord", "花王", 4, 5, 7, "flower", {}),
                 {buffEffect(TargetSelector::AllFriendliesSameRaceOther, 2, 1)}),

        // ---- 5 级 ----
        withBattlecry(plantTemplate("world_vine", "世界藤", 5, 8, 10, "vine", {Keyword::FastGrow}),
                      {buffEffect(TargetSelector::RandomFriendly, 4, 4)}),
        plantTemplate("ironwood", "铁木", 5, 7, 12, "tree", {Keyword::Thorns, Keyword::Reborn}),
        withAura(plantTemplate("sun_orchid", "向阳兰", 5, 6, 9, "flower", {Keyword::Shield}),
                 {buffEffect(TargetSelector::AllFriendliesOther, 2, 2)}),
        withAura(plantTemplate("tide_kelp", "潮汐藻", 5, 6, 11, "aquatic", {}),
                 {buffEffect(TargetSelector::AllFriendliesSameRaceOther, 1, 2)}),
        withDeathrattle(plantTemplate("mantrap", "巨捕", 5, 7, 7, "carnivorous", {Keyword::Poison}),
                        {damageEffect(TargetSelector::RandomEnemy, 4)}),

        // ---- 6 级 ----
        withBattlecry(plantTemplate("titan_bloom", "泰坦花", 6, 10, 12, "flower", {Keyword::FastGrow}),
                      {buffEffect(TargetSelector::AllFriendliesOther, 3, 3)}),
        withDeathrattle(plantTemplate("void_fungus", "虚空菌", 6, 9, 11, "fungus", {Keyword::Poison}),
                        {damageEffect(TargetSelector::AllEnemies, 3)}),
        withDeathrattle(plantTemplate("ancient_oak", "远古橡树", 6, 8, 15, "tree", {Keyword::Thorns}),
                        {summonEffect(plant_ids::kSeedling, 3)}),
        withAura(plantTemplate("world_tree", "世界树", 6, 7, 16, "tree", {}),
                 {buffEffect(TargetSelector::AllFriendliesSameRaceOther, 3, 3)}),
        plantTemplate("leviathan_weed", "巨藻", 6, 9, 13, "aquatic",
                      {Keyword::Thorns, Keyword::Reborn}),

        // ---- 召唤物 ----
        withBattlecry(asToken(plantTemplate("seedling", "幼苗", 1, 1, 1, "vine", {})),
                      {buffEffect(TargetSelector::RandomFriendlyOther, 1, 1)}),
    };
}
