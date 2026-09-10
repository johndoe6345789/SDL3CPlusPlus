// Pickup rules against ioq3's: bg_misc.c BG_CanItemBeGrabbed decides
// whether an item may be taken at all, and g_items.c Pickup_* decides
// what it gives. The quantities come from bg_itemlist.

#include "services/interfaces/workflow/quake3/q3_pickup_apply.hpp"

#include <gtest/gtest.h>

namespace impl = sdl3cpp::services::impl;

namespace {

impl::PickupTouchState Player(int health, int armor) {
    impl::PickupTouchState state;
    state.health    = health;
    state.armor     = armor;
    state.ammo      = nlohmann::json::object();
    state.inventory = nlohmann::json::object();
    return state;
}

}  // namespace

TEST(PickupRules, FullHealthLeavesAHealthPackAlone) {
    // The bug this guards: an item was consumed and sent away to
    // respawn even when it could give nothing.
    auto state = Player(100, 0);
    EXPECT_FALSE(impl::CanPickUpItem("item_health", state));
    EXPECT_FALSE(impl::CanPickUpItem("item_health_large", state));
}

TEST(PickupRules, SmallAndMegaHealthOverhealPastTheMaximum) {
    auto state = Player(100, 0);
    EXPECT_TRUE(impl::CanPickUpItem("item_health_small", state));
    EXPECT_TRUE(impl::CanPickUpItem("item_health_mega", state));

    impl::ApplyOnePickup("item_health_small", state);
    EXPECT_EQ(state.health, 105);

    auto mega = Player(100, 0);
    impl::ApplyOnePickup("item_health_mega", mega);
    EXPECT_EQ(mega.health, 200);
}

TEST(PickupRules, SmallHealthGivesFiveNotTwentyFive) {
    auto state = Player(50, 0);
    impl::ApplyOnePickup("item_health_small", state);
    EXPECT_EQ(state.health, 55);
}

TEST(PickupRules, HealthIsCappedAtTheMaximum) {
    auto state = Player(90, 0);
    impl::ApplyOnePickup("item_health", state);  // 25
    EXPECT_EQ(state.health, 100);
}

TEST(PickupRules, OverhealDecaysNoFurtherThanTwiceMax) {
    auto state = Player(195, 0);
    impl::ApplyOnePickup("item_health_mega", state);
    EXPECT_EQ(state.health, 200);
}

TEST(PickupRules, ArmourStacksToTwiceMaxHealth) {
    auto state = Player(100, 0);
    impl::ApplyOnePickup("item_armor_shard", state);
    EXPECT_EQ(state.armor, 5);
    impl::ApplyOnePickup("item_armor_combat", state);
    EXPECT_EQ(state.armor, 55);
    impl::ApplyOnePickup("item_armor_body", state);
    EXPECT_EQ(state.armor, 155);

    auto full = Player(100, 200);
    EXPECT_FALSE(impl::CanPickUpItem("item_armor_shard", full));
}

TEST(PickupRules, ArmourKeepsQuakesColours) {
    auto state = Player(100, 0);
    impl::ApplyOnePickup("item_armor_combat", state);
    EXPECT_EQ(state.armorType, "yellow");
    impl::ApplyOnePickup("item_armor_body", state);
    EXPECT_EQ(state.armorType, "red");
}

TEST(PickupRules, AWeaponIsAlwaysTakenAndTopsAmmoUp) {
    auto state = Player(100, 0);
    EXPECT_TRUE(impl::CanPickUpItem("weapon_railgun", state));

    impl::ApplyOnePickup("weapon_railgun", state);
    EXPECT_TRUE(state.inventory.value("weapon_railgun", false));
    EXPECT_EQ(state.ammo.value("weapon_railgun", 0), 10);

    // Already holding the full amount is worth a single round.
    impl::ApplyOnePickup("weapon_railgun", state);
    EXPECT_EQ(state.ammo.value("weapon_railgun", 0), 11);
}

TEST(PickupRules, WeaponAmmoMatchesQuakesQuantities) {
    auto state = Player(100, 0);
    impl::ApplyOnePickup("weapon_machinegun", state);
    EXPECT_EQ(state.ammo.value("weapon_machinegun", 0), 40);

    auto lightning = Player(100, 0);
    impl::ApplyOnePickup("weapon_lightning", lightning);
    EXPECT_EQ(lightning.ammo.value("weapon_lightning", 0), 100);
}

TEST(PickupRules, AmmoIsCappedAtTwoHundred) {
    auto state                       = Player(100, 0);
    state.ammo["weapon_machinegun"]  = 190;
    impl::ApplyOnePickup("ammo_bullets", state);  // 50
    EXPECT_EQ(state.ammo.value("weapon_machinegun", 0), 200);

    state.ammo["weapon_machinegun"] = 200;
    EXPECT_FALSE(impl::CanPickUpItem("ammo_bullets", state));
}

TEST(PickupRules, RespawnDelaysMatchQuake) {
    auto state = Player(50, 0);
    EXPECT_DOUBLE_EQ(impl::ApplyOnePickup("item_armor_shard", state), 25.0);
    EXPECT_DOUBLE_EQ(impl::ApplyOnePickup("item_health", state), 35.0);
    EXPECT_DOUBLE_EQ(impl::ApplyOnePickup("ammo_bullets", state), 40.0);
    EXPECT_DOUBLE_EQ(impl::ApplyOnePickup("weapon_railgun", state), 5.0);
}
