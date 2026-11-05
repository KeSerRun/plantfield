#pragma once

#include <string>
#include <vector>

#include "core/rng.h"
#include "game/garden.h"
#include "game/garden_loop.h"

class AiAgent {
public:
    void takeTurn(GardenState& g, GardenLoop& loop, Rng& rng, int round,
                  std::vector<std::string>& actions);
};
