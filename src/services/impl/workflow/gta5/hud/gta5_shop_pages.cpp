#include "services/interfaces/workflow/gta5/hud/gta5_shop_step.hpp"

#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

Gta5Menu WorkflowGta5ShopStep::Menu(WorkflowContext& context) const {
    Gta5Menu m;
    const bool garage = shops_[shop_].garage;
    m.hint = "ENTER TAKE   BACKSPACE BACK";
    m.selected = selected_;
    const auto inventory =
        context.Get<Gta5Inventory>("gta5.inventory", Gta5Inventory{});
    switch (page_) {
        case Gta5ShopPage::Root:
            m.title = garage ? "LOS SANTOS CUSTOMS" : "AMMU-NATION";
            m.items = garage ? std::vector<std::string>{"CHANGE CAR",
                                                        "RESPRAY", "LEAVE"}
                             : std::vector<std::string>{
                                   "WEAPONS", "FILL AMMUNITION",
                                   "BODY ARMOUR", "LEAVE"};
            break;
        case Gta5ShopPage::Cars:
            m.title = "CHANGE CAR";
            for (const auto& car : garage_.cars) m.items.push_back(car.name);
            break;
        case Gta5ShopPage::Colours:
            m.title = "RESPRAY";
            for (const auto& c : garage_.colours) m.items.push_back(c.name);
            break;
        case Gta5ShopPage::Weapons:
            m.title = "WEAPONS";
            for (std::size_t i = 0; i < weapons_.size(); ++i) {
                m.items.push_back(weapons_[i].name +
                                  (inventory.Owns(int(i)) ? "  OWNED" : ""));
            }
            break;
        case Gta5ShopPage::Closed:
            break;
    }
    return m;
}

void WorkflowGta5ShopStep::Navigate(WorkflowContext& context,
                                    const nlohmann::json* keys) {
    const int count = static_cast<int>(Menu(context).items.size());
    if (count == 0) return;
    if (Edge(keys, "Up")) selected_ = (selected_ + count - 1) % count;
    if (Edge(keys, "Down")) selected_ = (selected_ + 1) % count;
    if (Edge(keys, "Backspace")) {
        page_ = page_ == Gta5ShopPage::Root ? Gta5ShopPage::Closed
                                            : Gta5ShopPage::Root;
        selected_ = 0;
    } else if (Edge(keys, "Return")) {
        Choose(context);
    }
}

void WorkflowGta5ShopStep::Choose(WorkflowContext& context) {
    const auto pick = static_cast<std::size_t>(selected_);
    const bool garage = shops_[shop_].garage;
    const auto go = [&](Gta5ShopPage page) {
        page_ = page;
        selected_ = 0;
    };
    if (page_ == Gta5ShopPage::Cars) return SwapCar(context, pick);
    if (page_ == Gta5ShopPage::Colours) return Repaint(context, pick);
    if (page_ == Gta5ShopPage::Weapons) return GiveWeapon(context, pick);
    if (garage && pick == 0) return go(Gta5ShopPage::Cars);
    if (garage && pick == 1) return go(Gta5ShopPage::Colours);
    if (!garage && pick == 0) return go(Gta5ShopPage::Weapons);
    if (!garage && pick == 1) return FillAmmo(context);
    if (!garage && pick == 2) {
        context.Set<float>("gta5.player.armour", 100.f);
        return Remember(context);
    }
    page_ = Gta5ShopPage::Closed;  // LEAVE
}

}  // namespace sdl3cpp::services::impl
