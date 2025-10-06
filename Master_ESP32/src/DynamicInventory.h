#pragma once
#include <Arduino.h>
#include <vector>

// Status enumeration for consumable items
enum ItemStatus {
    STATUS_FULL = -1,
    STATUS_OK = 0,
    STATUS_LOW = 1,
    STATUS_OUT = 2
};

// Subcategory enumeration
enum Subcategory {
    SUBCATEGORY_TRAILER = 0,
    SUBCATEGORY_ESSENTIALS = 1,  
    SUBCATEGORY_OPTIONAL = 2
};

// Consumable item structure (with new livesInTrailer field)
struct ConsumableItem {
    String name;
    ItemStatus status;
    bool livesInTrailer;  // NEW FIELD - true if item stays in trailer, false if bought fresh each trip
    unsigned long lastUpdated;
    
    ConsumableItem() : status(STATUS_OK), livesInTrailer(true), lastUpdated(0) {}
    ConsumableItem(String n, ItemStatus s = STATUS_OK, bool trailer = true) : name(n), status(s), livesInTrailer(trailer), lastUpdated(millis()) {}
};

// Equipment item structure (with new livesInTrailer field for consistency)
struct EquipmentItem {
    String name;
    bool checked;
    bool packed;
    bool taking;
    bool livesInTrailer;  // NEW FIELD - always true for equipment, but keeps data structure consistent
    unsigned long lastUpdated;
    
    EquipmentItem() : checked(false), packed(false), taking(false), livesInTrailer(true), lastUpdated(0) {}
    EquipmentItem(String n, bool c = false, bool p = false, bool t = false) : name(n), checked(c), packed(p), taking(t), livesInTrailer(true), lastUpdated(millis()) {}
};

// Category structure
struct DynamicCategory {
    String name;
    String icon;
    bool isConsumable;
    Subcategory subcategory;
    std::vector<ConsumableItem> consumables;
    std::vector<EquipmentItem> equipment;
    
    DynamicCategory() : isConsumable(false), subcategory(SUBCATEGORY_TRAILER) {}
    DynamicCategory(String n, String i, bool consumable, Subcategory sub) : name(n), icon(i), isConsumable(consumable), subcategory(sub) {}
};

// Global inventory
extern std::vector<DynamicCategory> inventory;

// Helper function to get status name
inline const char* getStatusName(ItemStatus status) {
    switch(status) {
        case STATUS_FULL: return "Full";
        case STATUS_OK: return "OK";
        case STATUS_LOW: return "Low";
        case STATUS_OUT: return "Out";
        default: return "Unknown";
    }
}

// Smart defaults for livesInTrailer based on item names
bool shouldLiveInTrailer(const String& itemName, const String& categoryName);