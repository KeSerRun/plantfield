#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

class Rng {
public:
    Rng() : engine_(std::mt19937::default_seed) {}
    explicit Rng(std::uint32_t seed) : engine_(seed) {}

    int range(int lo, int hi) {
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(engine_);
    }

    template <typename T>
    const T& choice(const std::vector<T>& items) {
        return items[static_cast<std::size_t>(range(0, static_cast<int>(items.size()) - 1))];
    }

private:
    std::mt19937 engine_;
};
