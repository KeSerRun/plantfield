#pragma once

#include <string>
#include <vector>

#include "core/effect.h"

constexpr int kBoardLimit = 7;

enum class Keyword {
    Thorns,     // 荆棘：嘲讽，必须被优先攻击
    Shield,     // 护盾：免疫一次伤害
    FastGrow,   // 速生：每回合攻击两次
    Poison,     // 毒刺：击杀目标
    Reborn,     // 再生：死亡后复活一次
};

struct PlantTemplate {
    std::string id;
    std::string name;
    int tier = 1;
    int attack = 0;
    int health = 0;
    std::string race;
    bool token = false;
    std::vector<Keyword> keywords;
    std::vector<Effect> battlecry;    // 播种
    std::vector<Effect> deathrattle;  // 凋零
    std::vector<Effect> aura;         // 光合
};

struct Plant {
    int templateId = -1;
    int instanceId = -1;
    int attack = 0;
    int health = 0;
    int maxHealth = 0;
    bool hasThorns = false;
    bool hasShield = false;
    bool hasFastGrow = false;
    bool hasPoison = false;
    bool hasReborn = false;
    bool rebornUsed = false;
    bool destroyed = false;
    bool dead = false;
    int goldenLevel = 0;
};

inline Plant makePlant(const PlantTemplate& tmpl, int templateId) {
    Plant p;
    p.templateId = templateId;
    p.attack = tmpl.attack;
    p.health = tmpl.health;
    p.maxHealth = tmpl.health;
    for (Keyword k : tmpl.keywords) {
        switch (k) {
        case Keyword::Thorns:   p.hasThorns = true; break;
        case Keyword::Shield:   p.hasShield = true; break;
        case Keyword::FastGrow: p.hasFastGrow = true; break;
        case Keyword::Poison:   p.hasPoison = true; break;
        case Keyword::Reborn:   p.hasReborn = true; break;
        }
    }
    return p;
}
