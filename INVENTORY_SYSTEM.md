# Dynamic Inventory Tracker System

## Overview
ESP32-based camping inventory management system with dynamic add/remove capabilities, smart shopping lists, and persistent storage.

## Current Status (2025-10-05)

### ✅ Completed Features

#### 1. Dynamic Inventory System
- **Flexible data structure**: Uses `std::vector<DynamicCategory>` instead of static arrays
- **JSON persistence**: Saves to `/inventory.json` on SPIFFS
- **Runtime modification**: Add/remove items without recompiling
- **Auto-initialization**: Loads defaults on first boot

#### 2. Add/Remove Items
- **Add button**: ➕ at bottom of each category
- **Delete button**: ❌ on each item
- **Name validation**: 1-100 characters
- **Confirmation dialogs**: Prevents accidental deletion
- **Auto-reload**: Page refreshes after changes

#### 3. Shopping List Features
- **Auto-generation**: Shows all LOW and OUT items
- **Color-coded counts**: "7 OUT + 5 LOW" display
- **Selective restocking**: Checkboxes with filter buttons
  - Select All
  - Select Out Only
  - Select Low Only
  - Deselect All
- **Mark as restocked**: Sets selected items to FULL
- **Auto-refresh**: Updates when switching tabs
- **Export options**:
  - Copy to clipboard
  - Share (mobile with fallback to clipboard)
  - Download as text file

#### 4. User Interface
- **3 tabs**: Consumables, Equipment, Shopping
- **Summary stats**: Full/Low/Out counts with click-to-filter
- **Expandable categories**: 7 categories (3 consumable, 4 equipment)
- **Status tracking**:
  - Consumables: Full/OK/Low/Out buttons
  - Equipment: Checked/Packed checkboxes
- **Smooth scrolling**: Save button scrolls to top
- **Unsaved indicator**: Shows when changes need saving

## File Structure

```
Master_ESP32/
├── src/
│   └── main.cpp                    # Main application (1600+ lines)
├── include/
│   ├── VictronData.h              # Solar monitoring data structures
│   ├── DynamicInventory.h         # Dynamic inventory system ⭐ NEW
│   └── InventoryData.h            # Legacy static data (deprecated)
└── data/
    └── inventory.json             # Persistent storage (auto-created)
```

## Data Structure

### DynamicCategory
```cpp
struct DynamicCategory {
    String name;                        // e.g., "Kitchen"
    String icon;                        // Unicode emoji
    bool isConsumable;                  // true = status buttons, false = checkboxes
    std::vector<ConsumableItem> consumables;
    std::vector<EquipmentItem> equipment;

    int itemCount() {
        return isConsumable ? consumables.size() : equipment.size();
    }
};
```

### ConsumableItem
```cpp
struct ConsumableItem {
    String name;
    uint8_t status;  // 0=Full, 1=OK, 2=Low, 3=Out
};
```

### EquipmentItem
```cpp
struct EquipmentItem {
    String name;
    bool checked;
    bool packed;
};
```

## API Endpoints

| Endpoint | Method | Parameters | Description |
|----------|--------|------------|-------------|
| `/inventory` | GET | - | Main inventory page |
| `/inventory/set` | GET | cat, item, status | Set consumable status |
| `/inventory/check` | GET | cat, item, type, val | Set equipment checkbox |
| `/inventory/save` | GET | - | Save to SPIFFS |
| `/inventory/stats` | GET | - | Get summary counts (JSON) |
| `/inventory/shopping` | GET | - | Get shopping list (JSON) |
| `/inventory/resetall` | GET | - | Reset all to Full |
| `/inventory/restock` | POST | JSON array | Mark selected as Full |
| `/inventory/download` | GET | - | Download shopping list |
| `/inventory/add` | GET | cat, name | Add new item ⭐ NEW |
| `/inventory/remove` | GET | cat, item | Remove item ⭐ NEW |

## Default Categories

### Consumables (Status: Full/OK/Low/Out)
1. **Kitchen** 🍽️ - 11 items (roller towel, ziploc bags, etc.)
2. **Non-Perishables** 🥫 - 16 items (coffee, tea, sugar, etc.)
3. **General** 🔧 - 13 items (gas bottle, matches, batteries, etc.)

### Equipment (Checkboxes: Checked/Packed)
4. **Tents & Gazebos** ⛺ - 7 items
5. **Accessories** 🧰 - 8 items
6. **Sleeping Gear** 🛌 - 11 items
7. **Cooking Equipment** 🍳 - 16 items

## Storage Format (JSON)

```json
[
  {
    "name": "Kitchen",
    "icon": "🍽️",
    "isConsumable": true,
    "items": [
      {"name": "Roller towel", "status": 0},
      {"name": "Ziploc bags", "status": 1}
    ]
  },
  {
    "name": "Tents & Gazebos",
    "icon": "⛺",
    "isConsumable": false,
    "items": [
      {"name": "Canvas Tent & Poles", "checked": true, "packed": true}
    ]
  }
]
```

## Usage Instructions

### Adding Items While Camping
1. Open inventory page: `http://192.168.4.1/inventory`
2. Navigate to desired category (Consumables or Equipment tab)
3. Expand the category
4. Click **➕ Add Item** button at bottom
5. Enter item name (e.g., "Bug spray")
6. Click **💾 Save** to persist changes

### Removing Items
1. Find the item in its category
2. Click the **❌** delete button next to it
3. Confirm deletion
4. Click **💾 Save**

### Shopping List Workflow
1. Switch to **🛒 Shopping** tab
2. Review LOW and OUT items
3. Use filter buttons to select items:
   - **Select Out Only** - selects critical items
   - **Select Low Only** - selects items running low
   - **Select All** - selects everything
4. Click **✅ Mark Selected as Restocked** after shopping
5. Changes automatically saved

### Sharing Shopping List
- **Mobile**: Click **📤 Share** → opens native share dialog
- **Desktop/Unsupported**: Automatically copies to clipboard
- **Alternative**: Click **💾 Download** for text file

## Technical Details

### Memory Usage
- **RAM**: 13.9% (45,432 bytes / 327,680 bytes)
- **Flash**: 82.9% (1,085,966 bytes / 1,310,720 bytes)

### Persistence
- Storage location: `/inventory.json` on SPIFFS
- Auto-save on: Manual save button click
- Auto-load on: Boot
- Fallback: Initializes defaults if file missing

### JavaScript Functions
- `setStatus(cat, item, status)` - Set consumable status
- `setCheck(cat, item, type, val)` - Set equipment checkbox
- `saveInventory()` - Save to SPIFFS
- `addItem(cat)` - Prompt and add new item
- `removeItem(cat, item)` - Confirm and delete item
- `selectItems(mode)` - Filter shopping list selections
- `markRestocked()` - Bulk update to Full status
- `copyList()` - Copy to clipboard
- `shareList()` - Native share or clipboard fallback
- `downloadList()` - Download as .txt file
- `refreshShoppingList()` - AJAX reload shopping tab

## Known Limitations

1. **Item Name Length**: Maximum 100 characters
2. **Category Limit**: No UI to add/remove categories (only items)
3. **No Undo**: Deletion is permanent (save required)
4. **Single User**: No multi-device sync
5. **No Search**: Must browse categories manually

## Future Enhancements (Possible)

- [ ] Add/remove categories
- [ ] Search/filter across all items
- [ ] Export/import full inventory as JSON
- [ ] History/changelog of modifications
- [ ] Cloud sync (MQTT/HTTP)
- [ ] Barcode scanner integration
- [ ] Recipe-based shopping lists
- [ ] Auto-suggest items based on history

## Troubleshooting

### Items Not Saving
- Check serial monitor for SPIFFS errors
- Verify `/inventory.json` exists: Monitor shows "[INVENTORY] Saved X categories"
- Try "Reset All to Full" then save again

### Share Button Not Working
- **Expected on desktop** - Will copy to clipboard instead
- **On mobile**: Ensure using Chrome/Safari (not all browsers support Web Share API)
- **Fallback**: Use Copy or Download buttons

### Page Won't Load
- Check WiFi connection to "PowerMonitor"
- Verify IP: `http://192.168.4.1`
- Check serial output for web server errors

### Items Disappeared After Reboot
- SPIFFS may not be formatted - check serial output
- Re-add items and click **💾 Save**
- If persists, reflash with SPIFFS erase

## Development Notes

### Recent Changes (2025-10-05)
1. **Full migration** from `InventoryData.h` static arrays to `DynamicInventory.h` vectors
2. **JSON persistence** replacing binary format
3. **Add/remove handlers** with validation and error handling
4. **Smart share function** with progressive fallbacks
5. **All 12 handlers updated** to use vector access with proper bounds checking

### Git Commits
- `c023405` - Shopping list selective restocking fixes
- `29a4d31` - Dynamic inventory system with add/remove items

### Build Command
```bash
cd "C:\Trailer\CYD Build\Master_ESP32"
"C:\Users\LENOVO\.platformio\penv\Scripts\platformio.exe" run --target upload
```

## Credits

Built with Claude Code
Date: October 2025
Platform: ESP32 (Arduino Framework)
Libraries: SPIFFS, WebServer, WiFi, std::vector
