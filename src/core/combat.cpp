#include "core/combat.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "core/aura.h"
#include "core/format.h"

namespace {

constexpr int kLeft = 0;
constexpr int kRight = 1;
constexpr int kMaxSteps = 4000;

GameEvent makeEvent(EventType type, int sourceId, int targetId, int value, int value2 = 0,
                    bool poison = false) {
    GameEvent e;
    e.type = type;
    e.sourceId = sourceId;
    e.targetId = targetId;
    e.value = value;
    e.value2 = value2;
    e.poison = poison;
    return e;
}

}  // namespace

const PlantTemplate& CombatResolver::templateOf(const Plant& p) const {
    return (*templates_)[static_cast<std::size_t>(p.templateId)];
}

CombatResolver::Loc CombatResolver::locate(int instanceId) const {
    for (int side = kLeft; side <= kRight; ++side) {
        const std::vector<Plant>& board = sides_[side].board;
        for (std::size_t i = 0; i < board.size(); ++i) {
            if (board[i].instanceId == instanceId) {
                return Loc{side, static_cast<int>(i)};
            }
        }
    }
    return Loc{};
}

const Plant* CombatResolver::find(int instanceId) const {
    const Loc loc = locate(instanceId);
    if (loc.index < 0) {
        return nullptr;
    }
    return &sides_[loc.side].board[static_cast<std::size_t>(loc.index)];
}

bool CombatResolver::isDeadAt(int side, int index) const {
    const Plant& p = sides_[side].board[static_cast<std::size_t>(index)];
    return p.destroyed || effectiveHealthAt(side, index) <= 0;
}

bool CombatResolver::alive(int side, int index) const {
    const Plant& p = sides_[side].board[static_cast<std::size_t>(index)];
    if (p.dead || p.destroyed) {
        return false;
    }
    return effectiveHealthAt(side, index) > 0;
}

int CombatResolver::effectiveAttackAt(int side, int index) const {
    return effectiveAttack(sides_[side].board, *templates_, index);
}

int CombatResolver::effectiveHealthAt(int side, int index) const {
    return effectiveHealth(sides_[side].board, *templates_, index);
}

int CombatResolver::aliveCount(int side) const {
    int count = 0;
    for (int i = 0; i < static_cast<int>(sides_[side].board.size()); ++i) {
        if (alive(side, i)) {
            ++count;
        }
    }
    return count;
}

std::string CombatResolver::label(int side, int index) const {
    const Plant& p = sides_[side].board[static_cast<std::size_t>(index)];
    return templateOf(p).name + formatStats(effectiveAttackAt(side, index), effectiveHealthAt(side, index));
}

std::string CombatResolver::roster(int side) const {
    std::string out;
    for (int i = 0; i < static_cast<int>(sides_[side].board.size()); ++i) {
        if (!out.empty()) {
            out += "  ";
        }
        out += label(side, i);
    }
    return out;
}

void CombatResolver::log(const std::string& line) {
    if (frames_ != nullptr) {
        CombatFrame frame;
        frame.left = sides_[kLeft].board;
        frame.right = sides_[kRight].board;
        frame.line = line;
        frame.sourceId = currentSource_;
        frame.targetId = currentTarget_;
        frame.type = currentType_;
        frames_->push_back(frame);
    }
    if (verbose_) {
        std::cout << line << '\n';
    }
}

void CombatResolver::assignInstanceIds() {
    nextInstanceId_ = 0;
    for (int side = kLeft; side <= kRight; ++side) {
        order_[side].clear();
        cursor_[side] = 0;
        for (Plant& p : sides_[side].board) {
            p.instanceId = nextInstanceId_++;
            order_[side].push_back(p.instanceId);
        }
    }
}

int CombatResolver::nextInstanceId() {
    return nextInstanceId_++;
}

std::vector<int> CombatResolver::selectTargets(const Effect& e, int casterSide, int casterId) {
    std::vector<int> picked;
    const int enemySide = 1 - casterSide;

    switch (e.target) {
    case TargetSelector::Self:
        picked.push_back(casterId);
        break;
    case TargetSelector::Adjacent: {
        const Loc caster = locate(casterId);
        if (caster.index < 0) {
            break;
        }
        const std::vector<Plant>& board = sides_[caster.side].board;
        const int left = caster.index - 1;
        const int right = caster.index + 1;
        if (left >= 0 && alive(caster.side, left)) {
            picked.push_back(board[static_cast<std::size_t>(left)].instanceId);
        }
        if (right < static_cast<int>(board.size()) && alive(caster.side, right)) {
            picked.push_back(board[static_cast<std::size_t>(right)].instanceId);
        }
        break;
    }
    case TargetSelector::RandomFriendly:
    case TargetSelector::RandomFriendlyOther: {
        std::vector<int> pool;
        const std::vector<Plant>& board = sides_[casterSide].board;
        for (int i = 0; i < static_cast<int>(board.size()); ++i) {
            if (!alive(casterSide, i)) {
                continue;
            }
            if (e.target == TargetSelector::RandomFriendlyOther && board[i].instanceId == casterId) {
                continue;
            }
            pool.push_back(board[i].instanceId);
        }
        if (!pool.empty()) {
            picked.push_back(rng_.choice(pool));
        }
        break;
    }
    case TargetSelector::RandomEnemy: {
        std::vector<int> pool;
        const std::vector<Plant>& board = sides_[enemySide].board;
        for (int i = 0; i < static_cast<int>(board.size()); ++i) {
            if (alive(enemySide, i)) {
                pool.push_back(board[i].instanceId);
            }
        }
        if (!pool.empty()) {
            picked.push_back(rng_.choice(pool));
        }
        break;
    }
    case TargetSelector::AllEnemies: {
        const std::vector<Plant>& board = sides_[enemySide].board;
        for (int i = 0; i < static_cast<int>(board.size()); ++i) {
            if (alive(enemySide, i)) {
                picked.push_back(board[i].instanceId);
            }
        }
        break;
    }
    case TargetSelector::AllFriendlies:
    case TargetSelector::AllFriendliesOther: {
        const std::vector<Plant>& board = sides_[casterSide].board;
        for (int i = 0; i < static_cast<int>(board.size()); ++i) {
            if (!alive(casterSide, i)) {
                continue;
            }
            if (e.target == TargetSelector::AllFriendliesOther && board[i].instanceId == casterId) {
                continue;
            }
            picked.push_back(board[i].instanceId);
        }
        break;
    }
    case TargetSelector::AllFriendliesSameRace:
    case TargetSelector::AllFriendliesSameRaceOther: {
        const Plant* caster = find(casterId);
        const std::vector<Plant>& board = sides_[casterSide].board;
        for (int i = 0; i < static_cast<int>(board.size()); ++i) {
            if (!alive(casterSide, i) || caster == nullptr) {
                continue;
            }
            if (!sameRace(*templates_, *caster, board[i])) {
                continue;
            }
            if (e.target == TargetSelector::AllFriendliesSameRaceOther &&
                board[i].instanceId == casterId) {
                continue;
            }
            picked.push_back(board[i].instanceId);
        }
        break;
    }
    }
    return picked;
}

void CombatResolver::runEffects(const std::vector<Effect>& effects, int goldenLevel, int casterSide,
                                int casterId) {
    const int scale = goldenLevel > 0 ? 2 : 1;
    for (const Effect& e : effects) {
        if (e.action == EffectAction::Summon) {
            GameEvent ev = makeEvent(EventType::Summon, casterId, -1, e.summonTemplateId);
            ev.value2 = e.summonCount * scale;
            queue_.push(ev);
            continue;
        }
        for (int targetId : selectTargets(e, casterSide, casterId)) {
            if (e.action == EffectAction::Buff) {
                queue_.push(makeEvent(EventType::Buff, casterId, targetId, e.attackBonus * scale,
                                      e.healthBonus * scale));
            } else {
                queue_.push(makeEvent(EventType::Damage, casterId, targetId, e.damage * scale));
            }
        }
    }
}

void CombatResolver::markDead(int side, int index) {
    Plant& p = sides_[side].board[static_cast<std::size_t>(index)];
    if (p.dead) {
        return;
    }
    p.dead = true;
    queue_.push(makeEvent(EventType::Death, p.instanceId, -1, 0));
}

void CombatResolver::handleDamage(const GameEvent& e) {
    const Loc t = locate(e.targetId);
    if (t.index < 0) {
        return;
    }
    Plant& target = sides_[t.side].board[static_cast<std::size_t>(t.index)];
    if (target.dead || target.destroyed) {
        return;
    }

    if (target.hasShield) {
        target.hasShield = false;
        log("[护盾] " + templateOf(target).name + " 免疫了这次伤害");
        return;
    }

    target.health -= e.value;
    log("[伤害] " + templateOf(target).name + " 受到 " + std::to_string(e.value) +
        " 点伤害，剩余 " + std::to_string(std::max(effectiveHealthAt(t.side, t.index), 0)));

    if (e.poison && !isDeadAt(t.side, t.index) && effectiveHealthAt(t.side, t.index) > 0) {
        target.destroyed = true;
        log("[毒刺] " + templateOf(target).name + " 被毒杀");
    }
    if (isDeadAt(t.side, t.index)) {
        markDead(t.side, t.index);
    }
}

void CombatResolver::handleBuff(const GameEvent& e) {
    const Loc t = locate(e.targetId);
    if (t.index < 0) {
        return;
    }
    Plant& p = sides_[t.side].board[static_cast<std::size_t>(t.index)];
    if (p.dead) {
        return;
    }
    p.attack += e.value;
    p.maxHealth += e.value2;
    p.health += e.value2;
    log("[增益] " + templateOf(p).name + " 获得 +" + std::to_string(e.value) + "/+" +
        std::to_string(e.value2));
}

void CombatResolver::handleSummon(const GameEvent& e) {
    const Loc caster = locate(e.sourceId);
    if (caster.index < 0) {
        return;
    }
    const int templateId = e.value;
    if (templateId < 0 || templateId >= static_cast<int>(templates_->size())) {
        return;
    }

    for (int i = 0; i < e.value2; ++i) {
        std::vector<Plant>& board = sides_[caster.side].board;
        if (static_cast<int>(board.size()) >= kBoardLimit) {
            break;
        }
        Plant summoned = makePlant((*templates_)[static_cast<std::size_t>(templateId)], templateId);
        summoned.instanceId = nextInstanceId();
        board.push_back(summoned);
        order_[caster.side].push_back(summoned.instanceId);
        log("[召唤] " + templateOf(summoned).name + formatStats(summoned.attack, summoned.health));
        queue_.push(makeEvent(EventType::Battlecry, summoned.instanceId, -1, 0));
    }
}

void CombatResolver::handleTrigger(const GameEvent& e, bool battlecry) {
    const Loc l = locate(e.sourceId);
    if (l.index < 0) {
        return;
    }
    const Plant& p = sides_[l.side].board[static_cast<std::size_t>(l.index)];
    const PlantTemplate& tmpl = templateOf(p);
    runEffects(battlecry ? tmpl.battlecry : tmpl.deathrattle, p.goldenLevel, l.side, p.instanceId);
}

void CombatResolver::handleDeath(const GameEvent& e) {
    const Loc l = locate(e.sourceId);
    if (l.index < 0) {
        return;
    }
    const Plant& p = sides_[l.side].board[static_cast<std::size_t>(l.index)];
    if (!p.dead) {
        return;
    }

    log("[死亡] " + templateOf(p).name + " 死亡");

    if (!templateOf(p).deathrattle.empty()) {
        queue_.push(makeEvent(EventType::Deathrattle, p.instanceId, -1, 0));
    }
    if (p.hasReborn && !p.rebornUsed) {
        queue_.push(makeEvent(EventType::Reborn, p.instanceId, -1, 0));
    }
}

void CombatResolver::handleReborn(const GameEvent& e) {
    const Loc l = locate(e.sourceId);
    if (l.index < 0) {
        return;
    }
    Plant& p = sides_[l.side].board[static_cast<std::size_t>(l.index)];
    if (!p.hasReborn || p.rebornUsed) {
        return;
    }
    p.rebornUsed = true;
    p.destroyed = false;
    p.dead = false;
    p.health = 1;
    log("[再生] " + templateOf(p).name + " 复活（1 血）");
}

void CombatResolver::removeDead() {
    for (int side = kLeft; side <= kRight; ++side) {
        std::vector<Plant>& board = sides_[side].board;
        board.erase(std::remove_if(board.begin(), board.end(),
                                   [](const Plant& p) { return p.dead; }),
                    board.end());
    }
}

void CombatResolver::drainEvents() {
    while (!queue_.empty()) {
        const GameEvent e = queue_.pop();
        currentSource_ = e.sourceId;
        currentTarget_ = e.targetId;
        currentType_ = e.type;
        switch (e.type) {
        case EventType::Attack: {
            const Plant* source = find(e.sourceId);
            const Plant* target = find(e.targetId);
            if (source != nullptr && target != nullptr) {
                const Loc s = locate(e.sourceId);
                const Loc t = locate(e.targetId);
                log("[攻击] " + label(s.side, s.index) + " -> " + label(t.side, t.index));
            }
            break;
        }
        case EventType::Damage:
            handleDamage(e);
            break;
        case EventType::Buff:
            handleBuff(e);
            break;
        case EventType::Summon:
            handleSummon(e);
            break;
        case EventType::Battlecry:
            handleTrigger(e, true);
            break;
        case EventType::Deathrattle:
            handleTrigger(e, false);
            break;
        case EventType::Death:
            handleDeath(e);
            break;
        case EventType::Reborn:
            handleReborn(e);
            break;
        default:
            break;
        }
    }
    removeDead();
}

int CombatResolver::takeAttacker(int side) {
    const std::vector<int>& seq = order_[side];
    const std::size_t size = seq.size();
    if (size == 0) {
        return -1;
    }
    for (std::size_t k = 0; k < size; ++k) {
        const std::size_t pos = (cursor_[side] + k) % size;
        const Loc loc = locate(seq[pos]);
        if (loc.index >= 0 && alive(loc.side, loc.index)) {
            cursor_[side] = (pos + 1) % size;
            return seq[pos];
        }
    }
    return -1;
}

int CombatResolver::pickTarget(int attackerSide) {
    const int enemySide = 1 - attackerSide;
    std::vector<int> taunts;
    std::vector<int> others;
    const std::vector<Plant>& board = sides_[enemySide].board;
    for (int i = 0; i < static_cast<int>(board.size()); ++i) {
        if (!alive(enemySide, i)) {
            continue;
        }
        if (board[static_cast<std::size_t>(i)].hasThorns) {
            taunts.push_back(board[static_cast<std::size_t>(i)].instanceId);
        } else {
            others.push_back(board[static_cast<std::size_t>(i)].instanceId);
        }
    }
    const std::vector<int>& pool = taunts.empty() ? others : taunts;
    if (pool.empty()) {
        return -1;
    }
    return rng_.choice(pool);
}

void CombatResolver::attackStep(int side) {
    const int attackerId = takeAttacker(side);
    if (attackerId < 0) {
        return;
    }

    const Plant* attacker = find(attackerId);
    if (attacker == nullptr) {
        return;
    }
    const int attacks = attacker->hasFastGrow ? 2 : 1;

    for (int i = 0; i < attacks; ++i) {
        const Loc a = locate(attackerId);
        if (a.index < 0 || !alive(a.side, a.index)) {
            break;
        }

        const int enemy = 1 - a.side;
        if (aliveCount(enemy) == 0) {
            break;
        }

        const int targetId = pickTarget(a.side);
        if (targetId < 0) {
            break;
        }
        const Loc t = locate(targetId);
        if (t.index < 0) {
            break;
        }

        const int attackerDamage = effectiveAttackAt(a.side, a.index);
        const int retaliationDamage = effectiveAttackAt(t.side, t.index);
        const bool attackerPoison = sides_[a.side].board[static_cast<std::size_t>(a.index)].hasPoison;
        const bool targetPoison = sides_[t.side].board[static_cast<std::size_t>(t.index)].hasPoison;

        queue_.push(makeEvent(EventType::Attack, attackerId, targetId, 0));
        queue_.push(makeEvent(EventType::Damage, attackerId, targetId, attackerDamage, 0, attackerPoison));
        queue_.push(makeEvent(EventType::Damage, targetId, attackerId, retaliationDamage, 0, targetPoison));

        drainEvents();
    }
}

CombatResult CombatResolver::resolve(const CombatSide& left,
                                     const CombatSide& right,
                                     std::uint32_t seed,
                                     const std::vector<PlantTemplate>& templates,
                                     bool verbose,
                                     std::vector<CombatFrame>* frames) {
    sides_[kLeft] = left;
    sides_[kRight] = right;
    templates_ = &templates;
    verbose_ = verbose;
    frames_ = frames;
    currentSource_ = -1;
    currentTarget_ = -1;
    rng_ = Rng(seed);
    queue_ = EventQueue();
    assignInstanceIds();

    log("=== " + sides_[kLeft].name + "  VS  " + sides_[kRight].name +
        "  (seed=" + std::to_string(seed) + ") ===");
    log("[阵容] " + sides_[kLeft].name + "：" + roster(kLeft));
    log("[阵容] " + sides_[kRight].name + "：" + roster(kRight));
    log("");

    int attacker = sides_[kLeft].board.size() >= sides_[kRight].board.size() ? kLeft : kRight;
    if (sides_[kLeft].board.size() == sides_[kRight].board.size()) {
        attacker = rng_.range(kLeft, kRight);
    }

    int steps = 0;
    while (aliveCount(kLeft) > 0 && aliveCount(kRight) > 0 && steps < kMaxSteps) {
        attackStep(attacker);
        attacker = 1 - attacker;
        ++steps;
    }

    CombatResult result;
    result.leftSurvivors = aliveCount(kLeft);
    result.rightSurvivors = aliveCount(kRight);
    if (result.leftSurvivors > 0 && result.rightSurvivors == 0) {
        result.winner = CombatResult::Winner::Left;
    } else if (result.rightSurvivors > 0 && result.leftSurvivors == 0) {
        result.winner = CombatResult::Winner::Right;
    } else {
        result.winner = CombatResult::Winner::Draw;
    }

    log("");
    currentSource_ = -1;
    currentTarget_ = -1;
    if (result.winner == CombatResult::Winner::Draw) {
        log("[结果] 平局，双方全灭");
    } else {
        const std::string& winner = result.winner == CombatResult::Winner::Left
                                        ? sides_[kLeft].name
                                        : sides_[kRight].name;
        log("[结果] " + winner + " 胜利，存活 " + std::to_string(result.leftSurvivors) +
            " : " + std::to_string(result.rightSurvivors));
    }
    return result;
}
