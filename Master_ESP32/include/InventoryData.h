#pragma once
#include <Arduino.h>

// Item status for consumables
enum ItemStatus {
    STATUS_FULL = 0,
    STATUS_OK = 1,
    STATUS_LOW = 2,
    STATUS_OUT = 3
};

// Consumable item (Kitchen, Non-Perishables, General)
struct ConsumableItem {
    const char* name;
    uint8_t status;  // ItemStatus enum
};

// Equipment item (all other categories)
struct EquipmentItem {
    const char* name;
    bool checked;
    bool packed;
};

// Category structure
struct Category {
    const char* name;
    const char* icon;
    bool isConsumable;  // true = has status buttons, false = has checkboxes
    uint8_t itemCount;
    void* items;  // Points to either ConsumableItem[] or EquipmentItem[]
};

// Kitchen items
ConsumableItem kitchenItems[] = {
    {"Roller towel", STATUS_FULL},
    {"Ziploc bags", STATUS_OK},
    {"Wet wipes", STATUS_LOW},
    {"Glad wrap", STATUS_FULL},
    {"Tin foil", STATUS_OUT},
    {"Lappies", STATUS_OK},
    {"Sunlight", STATUS_FULL},
    {"Sponges", STATUS_OK},
    {"Black bags", STATUS_FULL},
    {"Paper Plates", STATUS_OK},
    {"Serviettes", STATUS_FULL}
};

// Non-Perishables items
ConsumableItem nonPerishableItems[] = {
    {"Cooking Oil", STATUS_OK},
    {"Coffee (instant)", STATUS_FULL},
    {"Normal Tea", STATUS_OK},
    {"Rooibos Tea", STATUS_FULL},
    {"Hot Chocolate", STATUS_OK},
    {"Sugar (sachets)", STATUS_LOW},
    {"Salt", STATUS_FULL},
    {"Pepper", STATUS_FULL},
    {"Mixed herbs/Bay Leaves", STATUS_OK},
    {"Peanut butter", STATUS_OUT},
    {"Filter Coffee", STATUS_OK},
    {"Marshmallows", STATUS_FULL},
    {"Tomato Sauce", STATUS_OK},
    {"5L Water", STATUS_FULL},
    {"Bouquet Garni", STATUS_OK},
    {"Lemon Juice", STATUS_OK}
};

// General items
ConsumableItem generalItems[] = {
    {"Gas Bottle", STATUS_FULL},
    {"Lamp oil", STATUS_OK},
    {"Gas lighter refill", STATUS_OK},
    {"Matches", STATUS_OK},
    {"Duct Tape", STATUS_FULL},
    {"Charcoal", STATUS_LOW},
    {"Wood", STATUS_OK},
    {"Paraffin", STATUS_OK},
    {"Citronella Candles", STATUS_OK},
    {"Mystical Fire Sachets", STATUS_FULL},
    {"Glow Sticks", STATUS_OK},
    {"Batteries", STATUS_LOW},
    {"Zip Ties", STATUS_FULL}
};

// Tents & Gazebos
EquipmentItem tentsItems[] = {
    {"Canvas Tent & Poles", true, true},
    {"Nylon Tent & Poles", false, false},
    {"2/4 Man Tent & Poles", false, false},
    {"Tent Pegs", true, true},
    {"Windbreak", false, false},
    {"Large Gazebo, poles, connector & sides", true, false},
    {"Pop-up gazebo incl grnd sheet & sides", false, false}
};

// Accessories
EquipmentItem accessoriesItems[] = {
    {"Mallet", true, true},
    {"Axe", true, true},
    {"Multitools", true, true},
    {"Knives", false, false},
    {"Butchers hooks", false, false},
    {"Backpacks", false, false},
    {"Games", false, false},
    {"First Aid", true, true}
};

// Sleeping Gear
EquipmentItem sleepingItems[] = {
    {"K-Way Mattress's", true, true},
    {"Blow-up Mattress's", false, false},
    {"Hand Pump", true, true},
    {"Electric Pump", true, true},
    {"Pillows", false, false},
    {"Pillow cases", false, false},
    {"Sleeping bags", true, false},
    {"Sheets", false, false},
    {"Duvet", false, false},
    {"Blankets", false, false},
    {"Towels", true, true}
};

// Cooking Equipment
EquipmentItem cookingItems[] = {
    {"Skottle", true, true},
    {"Gas stove top", true, true},
    {"Braai Grid - Scissor", true, true},
    {"Braai Grid - Large Flat", false, false},
    {"Pizza Oven & Plates", false, false},
    {"Braai Brush", true, true},
    {"Tongs", true, true},
    {"Fire Fan", true, true},
    {"Potjie & accessories", false, false},
    {"Cast Iron Bread Maker", false, false},
    {"Cooking Utensils", true, true},
    {"Braai Stand", true, true},
    {"Potjie Stand", false, false},
    {"Apron", false, false},
    {"Gas lighter", true, true},
    {"Gas fire starter", true, true}
};

// All categories
Category inventoryCategories[] = {
    {"Kitchen", "\u{1F37D}\u{FE0F}", true, 11, kitchenItems},
    {"Non-Perishables", "\u{1F96B}", true, 16, nonPerishableItems},
    {"General", "\u{1F527}", true, 13, generalItems},
    {"Tents & Gazebos", "\u{26FA}", false, 7, tentsItems},
    {"Accessories", "\u{1F9F0}", false, 8, accessoriesItems},
    {"Sleeping Gear", "\u{1F6CC}", false, 11, sleepingItems},
    {"Cooking Equipment", "\u{1F373}", false, 16, cookingItems}
};

const uint8_t INVENTORY_CATEGORY_COUNT = 7;

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
        case STATUS_FULL: return "#10b981";  // Green
        case STATUS_OK: return "#3b82f6";    // Blue
        case STATUS_LOW: return "#f59e0b";   // Orange
        case STATUS_OUT: return "#ef4444";   // Red
        default: return "#666";
    }
}
