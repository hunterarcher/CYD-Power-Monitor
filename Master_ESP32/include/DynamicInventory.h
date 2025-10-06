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
    bool taking;   // Third state for EXTRAS items (not taking this trip)

    EquipmentItem() : name(""), checked(false), packed(false), taking(true) {}
    EquipmentItem(String n, bool c, bool p) : name(n), checked(c), packed(p), taking(true) {}
    EquipmentItem(String n, bool c, bool p, bool t) : name(n), checked(c), packed(p), taking(t) {}
};

// Subcategory types for organization
enum Subcategory {
    SUBCATEGORY_NONE = 0,       // For consumables (no subcategory)
    SUBCATEGORY_TRAILER = 1,    // Items permanently in trailer  
    SUBCATEGORY_ESSENTIALS = 2, // Must pack every trip (toiletries, sunblock, etc.)
    SUBCATEGORY_OPTIONAL = 3    // Optional items (wood, extra tables, etc.)
};

// Category structure
struct DynamicCategory {
    String name;
    String icon;
    bool isConsumable;
    uint8_t subcategory;  // Subcategory type (TRAILER/EXTRAS for equipment)
    std::vector<ConsumableItem> consumables;
    std::vector<EquipmentItem> equipment;

    DynamicCategory() : name(""), icon(""), isConsumable(true), subcategory(SUBCATEGORY_NONE) {}
    DynamicCategory(String n, String i, bool c) : name(n), icon(i), isConsumable(c), subcategory(SUBCATEGORY_NONE) {}
    DynamicCategory(String n, String i, bool c, uint8_t s) : name(n), icon(i), isConsumable(c), subcategory(s) {}

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

    // ===== TRAILER CATEGORIES (Always there) =====
    
    // 🔧 Trailer Systems (TRAILER)
    DynamicCategory trailerSystems("Trailer Systems", "\u{1F527}", false, SUBCATEGORY_TRAILER);
    trailerSystems.equipment.push_back(EquipmentItem("EcoFlow", true, true, true));
    trailerSystems.equipment.push_back(EquipmentItem("Solar panel extension", true, true, true));
    trailerSystems.equipment.push_back(EquipmentItem("Hose to extend tap", true, true, true));
    trailerSystems.equipment.push_back(EquipmentItem("Stand for Kitchen", true, true, true));
    inventory.push_back(trailerSystems);

    // 🍳 Kitchen Basics (TRAILER)
    DynamicCategory kitchenBasics("Kitchen Basics", "\u{1F373}", false, SUBCATEGORY_TRAILER);
    kitchenBasics.equipment.push_back(EquipmentItem("Pots and Pans", true, true, true));
    kitchenBasics.equipment.push_back(EquipmentItem("Kettle", true, true, true));
    kitchenBasics.equipment.push_back(EquipmentItem("Cooking Utensils", true, true, true));
    kitchenBasics.equipment.push_back(EquipmentItem("Skottle", true, true, true));
    kitchenBasics.equipment.push_back(EquipmentItem("Gas stove top", true, true, true));
    kitchenBasics.equipment.push_back(EquipmentItem("Braai Grid - Scissor", true, true, true));
    kitchenBasics.equipment.push_back(EquipmentItem("Tongs", true, true, true));
    kitchenBasics.equipment.push_back(EquipmentItem("Braai Brush", true, true, true));
    kitchenBasics.equipment.push_back(EquipmentItem("Fire Fan", true, true, true));
    kitchenBasics.equipment.push_back(EquipmentItem("Gas lighter", true, true, true));
    inventory.push_back(kitchenBasics);

    // 🍽️ Crockery & Cutlery (TRAILER)
    DynamicCategory crockery("Crockery and Cutlery", "\u{1F37D}", false, SUBCATEGORY_TRAILER);
    crockery.equipment.push_back(EquipmentItem("Bowls", true, true, true));
    crockery.equipment.push_back(EquipmentItem("Metal Cups", true, true, true));
    crockery.equipment.push_back(EquipmentItem("Mugs", true, true, true));
    crockery.equipment.push_back(EquipmentItem("Knives", true, true, true));
    crockery.equipment.push_back(EquipmentItem("Forks", true, true, true));
    crockery.equipment.push_back(EquipmentItem("Spoons", true, true, true));
    crockery.equipment.push_back(EquipmentItem("Teaspoons", true, true, true));
    crockery.equipment.push_back(EquipmentItem("Sharp Knives", true, true, true));
    crockery.equipment.push_back(EquipmentItem("Scissors", true, true, true));
    crockery.equipment.push_back(EquipmentItem("Paper Plate Holder", true, true, true));
    inventory.push_back(crockery);

    // 🛠️ Tools and Utilities (TRAILER)
    DynamicCategory tools("Tools and Utilities", "\u{1F6E0}", false, SUBCATEGORY_TRAILER);
    tools.equipment.push_back(EquipmentItem("Mallet", true, true, true));
    tools.equipment.push_back(EquipmentItem("Axe", true, true, true));
    tools.equipment.push_back(EquipmentItem("Multitools", true, true, true));
    tools.equipment.push_back(EquipmentItem("Extension Cords", true, true, true));
    tools.equipment.push_back(EquipmentItem("Multi-adapters", true, true, true));
    tools.equipment.push_back(EquipmentItem("Washing bowl - collapsible", true, true, true));
    tools.equipment.push_back(EquipmentItem("Dish cloths", true, true, true));
    inventory.push_back(tools);

    // ===== ESSENTIALS CATEGORIES (Must pack every trip) =====

    // 👤 Personal Items (ESSENTIALS)
    DynamicCategory personal("Personal Items", "\u{1F9F4}", false, SUBCATEGORY_ESSENTIALS);
    personal.equipment.push_back(EquipmentItem("Toiletries", false, false, true));
    personal.equipment.push_back(EquipmentItem("Sunblock", false, false, true));
    personal.equipment.push_back(EquipmentItem("Meds", false, false, true));
    personal.equipment.push_back(EquipmentItem("Clothes", false, false, true));
    personal.equipment.push_back(EquipmentItem("Shoes", false, false, true));
    personal.equipment.push_back(EquipmentItem("Cozzies", false, false, true));
    inventory.push_back(personal);

    // 🛏️ Sleep and Comfort (ESSENTIALS)
    DynamicCategory sleepComfort("Sleep and Comfort", "\u{1F6CF}", false, SUBCATEGORY_ESSENTIALS);
    sleepComfort.equipment.push_back(EquipmentItem("Duvet", false, false, true));
    sleepComfort.equipment.push_back(EquipmentItem("Duvet Cover", false, false, true));
    sleepComfort.equipment.push_back(EquipmentItem("Pillows", false, false, true));
    sleepComfort.equipment.push_back(EquipmentItem("Beach towels", false, false, true));
    inventory.push_back(sleepComfort);

    // 🚨 Safety and Fun (ESSENTIALS)
    DynamicCategory safety("Safety and Fun", "\u{1F6A8}", false, SUBCATEGORY_ESSENTIALS);
    safety.equipment.push_back(EquipmentItem("First aid", false, false, true));
    safety.equipment.push_back(EquipmentItem("Life jacket", false, false, true));
    safety.equipment.push_back(EquipmentItem("Walkie Talkies + chargers", false, false, true));
    safety.equipment.push_back(EquipmentItem("Books", false, false, true));
    safety.equipment.push_back(EquipmentItem("Toys", false, false, true));
    safety.equipment.push_back(EquipmentItem("Buckets", false, false, true));
    safety.equipment.push_back(EquipmentItem("Boat and ore", false, false, true));
    safety.equipment.push_back(EquipmentItem("Bikes/helmets", false, false, true));
    inventory.push_back(safety);

    // ===== OPTIONAL CATEGORIES (Trip-dependent) =====

    // 🏕️ Shelter and Setup (OPTIONAL)
    DynamicCategory shelter("Shelter and Setup", "\u{26FA}", false, SUBCATEGORY_OPTIONAL);
    shelter.equipment.push_back(EquipmentItem("Nylon Tent + Poles", false, false, false));
    shelter.equipment.push_back(EquipmentItem("2/4 Man Tent + Poles", false, false, false));
    shelter.equipment.push_back(EquipmentItem("Windbreak", false, false, false));
    shelter.equipment.push_back(EquipmentItem("Pop-up gazebo + ground sheets", false, false, false));
    shelter.equipment.push_back(EquipmentItem("Ground sheets", false, false, false));
    inventory.push_back(shelter);

    // 🛋️ Extra Furniture (OPTIONAL)
    DynamicCategory furniture("Extra Furniture", "\u{1FA91}", false, SUBCATEGORY_OPTIONAL);
    furniture.equipment.push_back(EquipmentItem("Large foldup table", false, false, false));
    furniture.equipment.push_back(EquipmentItem("Small tables", false, false, false));
    furniture.equipment.push_back(EquipmentItem("Table + Benches", false, false, false));
    furniture.equipment.push_back(EquipmentItem("Hammock", false, false, false));
    inventory.push_back(furniture);

    // 🔥 Cooking Extras (OPTIONAL)
    DynamicCategory cookingExtras("Cooking Extras", "\u{1F525}", false, SUBCATEGORY_OPTIONAL);
    cookingExtras.equipment.push_back(EquipmentItem("Pizza Oven + Plates", false, false, false));
    cookingExtras.equipment.push_back(EquipmentItem("Potjie + accessories", false, false, false));
    cookingExtras.equipment.push_back(EquipmentItem("Braai Grid - Large Flat", false, false, false));
    cookingExtras.equipment.push_back(EquipmentItem("Cast Iron Bread Maker", false, false, false));
    cookingExtras.equipment.push_back(EquipmentItem("Potjie Stand", false, false, false));
    inventory.push_back(cookingExtras);

    // 💡 Extra Lighting (OPTIONAL)
    DynamicCategory lighting("Extra Lighting", "\u{1F4A1}", false, SUBCATEGORY_OPTIONAL);
    lighting.equipment.push_back(EquipmentItem("Solar Lanterns", false, false, false));
    lighting.equipment.push_back(EquipmentItem("Torches", false, false, false));
    lighting.equipment.push_back(EquipmentItem("Solar Jars", false, false, false));
    lighting.equipment.push_back(EquipmentItem("Paraffin Lamp", false, false, false));
    lighting.equipment.push_back(EquipmentItem("Solar Fairy Lights", false, false, false));
    inventory.push_back(lighting);

    // 🎒 Gear and Storage (OPTIONAL)
    DynamicCategory gearStorage("Gear and Storage", "\u{1F392}", false, SUBCATEGORY_OPTIONAL);
    gearStorage.equipment.push_back(EquipmentItem("Backpacks", false, false, false));
    gearStorage.equipment.push_back(EquipmentItem("Games", false, false, false));
    gearStorage.equipment.push_back(EquipmentItem("Floaties", false, false, false));
    gearStorage.equipment.push_back(EquipmentItem("Water container - soft", false, false, false));
    gearStorage.equipment.push_back(EquipmentItem("Cooler Box", false, false, false));
    gearStorage.equipment.push_back(EquipmentItem("Ice bricks", false, false, false));
    inventory.push_back(gearStorage);
}
