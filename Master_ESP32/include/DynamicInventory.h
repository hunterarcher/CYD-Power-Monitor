#pragma once
#include <Arduino.h>
#include <vector>
#include <algorithm>

// Item status for consumables
enum ItemStatus {
    STATUS_FULL = 0,
    STATUS_OK = 1,
    STATUS_LOW = 2,
    STATUS_OUT = 3
};

// Consumable item (Kitchen, Non-Perishables, General)
struct ConsumableItem {
    String name;
    uint8_t status;

    ConsumableItem() : name(""), status(STATUS_FULL) {}
    ConsumableItem(String n, uint8_t s) : name(n), status(s) {}
};

// Equipment item (all other categories)
struct EquipmentItem {
    String name;
    bool checked;
    bool packed;

    EquipmentItem() : name(""), checked(false), packed(false) {}
    EquipmentItem(String n, bool c, bool p) : name(n), checked(c), packed(p) {}
};

// Category structure
struct DynamicCategory {
    String name;
    String icon;
    bool isConsumable;
    std::vector<ConsumableItem> consumables;
    std::vector<EquipmentItem> equipment;

    DynamicCategory() : name(""), icon(""), isConsumable(true) {}
    DynamicCategory(String n, String i, bool c) : name(n), icon(i), isConsumable(c) {}

    int itemCount() {
        return isConsumable ? consumables.size() : equipment.size();
    }
};

// Global inventory storage
std::vector<DynamicCategory> inventory;

// Helper functions
const char* getStatusName(uint8_t status) {
    switch(status) {
        case STATUS_FULL: return "Full";
        case STATUS_OK: return "OK";
        case STATUS_LOW: return "Low";
        case STATUS_OUT: return "Out";
        default: return "Unknown";
    }
}

const char* getStatusColor(uint8_t status) {
    switch(status) {
        case STATUS_FULL: return "#10b981";
        case STATUS_OK: return "#3b82f6";
        case STATUS_LOW: return "#f59e0b";
        case STATUS_OUT: return "#ef4444";
        default: return "#666";
    }
}

// Sort items alphabetically within a category (case-insensitive)
void sortCategoryItems(DynamicCategory& category) {
    if (category.isConsumable) {
        std::sort(category.consumables.begin(), category.consumables.end(),
                  [](const ConsumableItem& a, const ConsumableItem& b) {
                      String aLower = a.name;
                      String bLower = b.name;
                      aLower.toLowerCase();
                      bLower.toLowerCase();
                      return aLower < bLower;
                  });
    } else {
        std::sort(category.equipment.begin(), category.equipment.end(),
                  [](const EquipmentItem& a, const EquipmentItem& b) {
                      String aLower = a.name;
                      String bLower = b.name;
                      aLower.toLowerCase();
                      bLower.toLowerCase();
                      return aLower < bLower;
                  });
    }
}

// Sort all categories alphabetically
void sortAllInventory() {
    for (size_t i = 0; i < inventory.size(); i++) {
        sortCategoryItems(inventory[i]);
    }
}

// Initialize with default items
void initializeDefaultInventory() {
    inventory.clear();

    // Kitchen
    DynamicCategory kitchen("Kitchen", "\u{1F37D}\u{FE0F}", true);
    kitchen.consumables.push_back(ConsumableItem("Roller towel", STATUS_FULL));
    kitchen.consumables.push_back(ConsumableItem("Ziploc bags", STATUS_OK));
    kitchen.consumables.push_back(ConsumableItem("Wet wipes", STATUS_LOW));
    kitchen.consumables.push_back(ConsumableItem("Glad wrap", STATUS_FULL));
    kitchen.consumables.push_back(ConsumableItem("Tin foil", STATUS_OUT));
    kitchen.consumables.push_back(ConsumableItem("Lappies", STATUS_OK));
    kitchen.consumables.push_back(ConsumableItem("Sunlight", STATUS_FULL));
    kitchen.consumables.push_back(ConsumableItem("Sponges", STATUS_OK));
    kitchen.consumables.push_back(ConsumableItem("Black bags", STATUS_FULL));
    kitchen.consumables.push_back(ConsumableItem("Paper Plates", STATUS_OK));
    kitchen.consumables.push_back(ConsumableItem("Serviettes", STATUS_FULL));
    inventory.push_back(kitchen);

    // Non-Perishables
    DynamicCategory nonPerishable("Non-Perishables", "\u{1F96B}", true);
    nonPerishable.consumables.push_back(ConsumableItem("Cooking Oil", STATUS_OK));
    nonPerishable.consumables.push_back(ConsumableItem("Coffee (instant)", STATUS_FULL));
    nonPerishable.consumables.push_back(ConsumableItem("Normal Tea", STATUS_OK));
    nonPerishable.consumables.push_back(ConsumableItem("Rooibos Tea", STATUS_FULL));
    nonPerishable.consumables.push_back(ConsumableItem("Hot Chocolate", STATUS_OK));
    nonPerishable.consumables.push_back(ConsumableItem("Sugar (sachets)", STATUS_LOW));
    nonPerishable.consumables.push_back(ConsumableItem("Salt", STATUS_FULL));
    nonPerishable.consumables.push_back(ConsumableItem("Pepper", STATUS_FULL));
    nonPerishable.consumables.push_back(ConsumableItem("Mixed herbs/Bay Leaves", STATUS_OK));
    nonPerishable.consumables.push_back(ConsumableItem("Peanut butter", STATUS_OUT));
    nonPerishable.consumables.push_back(ConsumableItem("Filter Coffee", STATUS_OK));
    nonPerishable.consumables.push_back(ConsumableItem("Marshmallows", STATUS_FULL));
    nonPerishable.consumables.push_back(ConsumableItem("Tomato Sauce", STATUS_OK));
    nonPerishable.consumables.push_back(ConsumableItem("5L Water", STATUS_FULL));
    nonPerishable.consumables.push_back(ConsumableItem("Bouquet Garni", STATUS_OK));
    nonPerishable.consumables.push_back(ConsumableItem("Lemon Juice", STATUS_OK));
    inventory.push_back(nonPerishable);

    // General
    DynamicCategory general("General", "\u{1F527}", true);
    general.consumables.push_back(ConsumableItem("Gas Bottle", STATUS_FULL));
    general.consumables.push_back(ConsumableItem("Lamp oil", STATUS_OK));
    general.consumables.push_back(ConsumableItem("Gas lighter refill", STATUS_OK));
    general.consumables.push_back(ConsumableItem("Matches", STATUS_OK));
    general.consumables.push_back(ConsumableItem("Duct Tape", STATUS_FULL));
    general.consumables.push_back(ConsumableItem("Charcoal", STATUS_LOW));
    general.consumables.push_back(ConsumableItem("Wood", STATUS_OK));
    general.consumables.push_back(ConsumableItem("Paraffin", STATUS_OK));
    general.consumables.push_back(ConsumableItem("Citronella Candles", STATUS_OK));
    general.consumables.push_back(ConsumableItem("Mystical Fire Sachets", STATUS_FULL));
    general.consumables.push_back(ConsumableItem("Glow Sticks", STATUS_OK));
    general.consumables.push_back(ConsumableItem("Batteries", STATUS_LOW));
    general.consumables.push_back(ConsumableItem("Zip Ties", STATUS_FULL));
    inventory.push_back(general);

    // Tents & Gazebos
    DynamicCategory tents("Tents & Gazebos", "\u{26FA}", false);
    tents.equipment.push_back(EquipmentItem("Canvas Tent & Poles", true, true));
    tents.equipment.push_back(EquipmentItem("Nylon Tent & Poles", false, false));
    tents.equipment.push_back(EquipmentItem("2/4 Man Tent & Poles", false, false));
    tents.equipment.push_back(EquipmentItem("Tent Pegs", true, true));
    tents.equipment.push_back(EquipmentItem("Windbreak", false, false));
    tents.equipment.push_back(EquipmentItem("Large Gazebo, poles, connector & sides", true, false));
    tents.equipment.push_back(EquipmentItem("Pop-up gazebo incl grnd sheet & sides", false, false));
    inventory.push_back(tents);

    // Accessories
    DynamicCategory accessories("Accessories", "\u{1F9F0}", false);
    accessories.equipment.push_back(EquipmentItem("Mallet", true, true));
    accessories.equipment.push_back(EquipmentItem("Axe", true, true));
    accessories.equipment.push_back(EquipmentItem("Multitools", true, true));
    accessories.equipment.push_back(EquipmentItem("Knives", false, false));
    accessories.equipment.push_back(EquipmentItem("Butchers hooks", false, false));
    accessories.equipment.push_back(EquipmentItem("Backpacks", false, false));
    accessories.equipment.push_back(EquipmentItem("Games", false, false));
    accessories.equipment.push_back(EquipmentItem("First Aid", true, true));
    inventory.push_back(accessories);

    // Sleeping Gear
    DynamicCategory sleeping("Sleeping Gear", "\u{1F6CC}", false);
    sleeping.equipment.push_back(EquipmentItem("K-Way Mattress's", true, true));
    sleeping.equipment.push_back(EquipmentItem("Blow-up Mattress's", false, false));
    sleeping.equipment.push_back(EquipmentItem("Hand Pump", true, true));
    sleeping.equipment.push_back(EquipmentItem("Electric Pump", true, true));
    sleeping.equipment.push_back(EquipmentItem("Pillows", false, false));
    sleeping.equipment.push_back(EquipmentItem("Pillow cases", false, false));
    sleeping.equipment.push_back(EquipmentItem("Sleeping bags", true, false));
    sleeping.equipment.push_back(EquipmentItem("Sheets", false, false));
    sleeping.equipment.push_back(EquipmentItem("Duvet", false, false));
    sleeping.equipment.push_back(EquipmentItem("Blankets", false, false));
    sleeping.equipment.push_back(EquipmentItem("Towels", true, true));
    inventory.push_back(sleeping);

    // Cooking Equipment
    DynamicCategory cooking("Cooking Equipment", "\u{1F373}", false);
    cooking.equipment.push_back(EquipmentItem("Skottle", true, true));
    cooking.equipment.push_back(EquipmentItem("Gas stove top", true, true));
    cooking.equipment.push_back(EquipmentItem("Braai Grid - Scissor", true, true));
    cooking.equipment.push_back(EquipmentItem("Braai Grid - Large Flat", false, false));
    cooking.equipment.push_back(EquipmentItem("Pizza Oven & Plates", false, false));
    cooking.equipment.push_back(EquipmentItem("Braai Brush", true, true));
    cooking.equipment.push_back(EquipmentItem("Tongs", true, true));
    cooking.equipment.push_back(EquipmentItem("Fire Fan", true, true));
    cooking.equipment.push_back(EquipmentItem("Potjie & accessories", false, false));
    cooking.equipment.push_back(EquipmentItem("Cast Iron Bread Maker", false, false));
    cooking.equipment.push_back(EquipmentItem("Cooking Utensils", true, true));
    cooking.equipment.push_back(EquipmentItem("Braai Stand", true, true));
    cooking.equipment.push_back(EquipmentItem("Potjie Stand", false, false));
    cooking.equipment.push_back(EquipmentItem("Apron", false, false));
    cooking.equipment.push_back(EquipmentItem("Gas lighter", true, true));
    cooking.equipment.push_back(EquipmentItem("Gas fire starter", true, true));
    inventory.push_back(cooking);
}
