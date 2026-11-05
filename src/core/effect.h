#pragma once

enum class EffectAction {
    Buff,
    Damage,
    Summon,
};

enum class TargetSelector {
    Self,
    Adjacent,
    RandomEnemy,
    AllEnemies,
    RandomFriendly,
    RandomFriendlyOther,
    AllFriendlies,
    AllFriendliesOther,
    AllFriendliesSameRace,
    AllFriendliesSameRaceOther,
};

struct Effect {
    EffectAction action = EffectAction::Buff;
    TargetSelector target = TargetSelector::Self;
    int damage = 0;
    int attackBonus = 0;
    int healthBonus = 0;
    int summonTemplateId = -1;
    int summonCount = 1;
};

inline Effect buffEffect(TargetSelector target, int attack, int health) {
    Effect e;
    e.action = EffectAction::Buff;
    e.target = target;
    e.attackBonus = attack;
    e.healthBonus = health;
    return e;
}

inline Effect damageEffect(TargetSelector target, int damage) {
    Effect e;
    e.action = EffectAction::Damage;
    e.target = target;
    e.damage = damage;
    return e;
}

inline Effect summonEffect(int templateId, int count) {
    Effect e;
    e.action = EffectAction::Summon;
    e.target = TargetSelector::Self;
    e.summonTemplateId = templateId;
    e.summonCount = count;
    return e;
}
