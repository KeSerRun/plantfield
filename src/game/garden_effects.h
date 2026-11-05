#pragma once

#include <vector>

#include "core/effect.h"
#include "core/plant.h"
#include "core/rng.h"
#include "game/garden.h"

void runGardenEffects(GardenState& g, int casterIndex, const std::vector<Effect>& effects,
                      const std::vector<PlantTemplate>& pool, Rng& rng, int depth);
