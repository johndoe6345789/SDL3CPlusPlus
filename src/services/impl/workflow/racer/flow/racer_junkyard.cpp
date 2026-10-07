#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kJunkOffers = 5;
constexpr float kJunkPriceShare = 0.4f;  // of the new price, before wear
constexpr float kDroidRepair = 0.06f;    // health each droid restores

/// A small repeatable generator: the same stock until the next race.
std::uint32_t Next(std::uint32_t& seed) {
    seed = seed * 1664525u + 1013904223u;
    return seed >> 8;
}

}  // namespace

int RacerStockedLevel(int type, int podiums) {
    int level = 0;
    for (int l = 0; l <= kRacerUpgradeMax; ++l) {
        if (RacerPart(type, l).podiumsNeeded <= podiums) level = l;
    }
    return level;
}

std::vector<RacerJunkOffer> RacerJunkyardStock(const RacerProfile& profile) {
    std::uint32_t seed = 7919u * static_cast<std::uint32_t>(
                                     profile.racesRun + 1) + 13u;
    std::vector<RacerJunkOffer> stock;
    for (int i = 0; i < kJunkOffers; ++i) {
        RacerJunkOffer offer;
        offer.type = static_cast<int>(Next(seed) % kRacerUpgradeCount);
        // Scrap can be a step ahead of what Watto sells new.
        const int stocked = RacerStockedLevel(offer.type, profile.podiums);
        const int best = std::min(kRacerUpgradeMax, stocked + 1);
        offer.level = 1 + static_cast<int>(Next(seed) % best);
        offer.health = 0.35f + 0.01f * static_cast<float>(Next(seed) % 50);
        const float price = kJunkPriceShare * offer.health *
                            RacerPart(offer.type, offer.level).price;
        offer.price = std::max(10, 10 * static_cast<int>(price / 10.f));
        stock.push_back(offer);
    }
    return stock;
}

std::string WearAndRepairRacerParts(RacerProfile& profile, float damage,
                                    bool overheated) {
    // Every part wears a little each race, more for a battered pod;
    // running hot wears the coolers and injectors besides.
    for (int type = 0; type < kRacerUpgradeCount; ++type) {
        float wear = 0.03f + 0.2f * std::clamp(damage, 0.f, 1.f);
        if (overheated && (type == 2 || type == 5)) wear += 0.1f;
        float& health = profile.health[type];
        health = std::max(0.f, health - wear);
        health = std::min(1.f, health + kDroidRepair * profile.pitDroids);
    }
    float least = 1.f;
    for (float h : profile.health) least = std::min(least, h);
    const int droids = profile.pitDroids;
    return std::to_string(droids) + (droids == 1 ? " PIT DROID" :
                                                   " PIT DROIDS") +
           " REPAIRED: WORST PART AT " +
           std::to_string(static_cast<int>(std::round(100.f * least))) + "%";
}

}  // namespace sdl3cpp::services::impl
