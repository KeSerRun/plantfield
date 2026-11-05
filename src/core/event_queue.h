#pragma once

#include <deque>

enum class EventType {
    Attack,
    Damage,
    Death,
    Deathrattle,
    Battlecry,
    Summon,
    Buff,
    ShieldPop,
    Reborn,
};

struct GameEvent {
    EventType type;
    int sourceId = -1;
    int targetId = -1;
    int value = 0;
    int value2 = 0;
    int priority = 0;
    bool poison = false;
};

class EventQueue {
public:
    void push(const GameEvent& e) { queue_.push_back(e); }

    bool empty() const { return queue_.empty(); }

    GameEvent pop() {
        GameEvent e = queue_.front();
        queue_.pop_front();
        return e;
    }

private:
    std::deque<GameEvent> queue_;
};
