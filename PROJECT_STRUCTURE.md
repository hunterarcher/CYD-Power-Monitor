# Trailer Power Monitor - Project Structure

**Date:** 2025-10-06  
**Status:** Production Ready  
**Flash Usage:** 90.2% (1,182,762 / 1,310,720 bytes)

---

## 🎯 Project Overview

Complete RV/Trailer power monitoring and inventory management system using ESP32 devices with ESP-NOW communication and BLE device integration.

### **Key Features**
✅ **Multi-Device Monitoring:** Victron solar system, EcoFlow battery, fridge temperatures  
✅ **Inventory Management:** Dynamic camping equipment tracking with smart categories  
✅ **Shopping Lists:** Auto-generated from consumable status  
✅ **Fridge Control:** Set temperatures, ECO mode, battery protection via web interface  
✅ **Smart Backup System:** Daily/weekly/monthly/quarterly retention (158KB total)  
✅ **iPad-Optimized UI:** Card-based dashboard with consistent navigation  

---

## 📁 Project Structure

### **🔧 Active ESP32 Projects**
```
├── Master_ESP32/           # 🎛️  Main hub - Web dashboard + ESP-NOW coordinator
├── Victron_ESP32/          # ☀️  Solar system monitoring (BLE scanning)
├── EcoFlow_ESP32/          # 🔋  Battery pack monitoring (BLE scanning)  
├── Fridge_ESP32/           # ❄️  Fridge control (BLE connection)
```

### **📊 Data & Analysis**
```
├── EcoFlow Data/           # 📈  BLE reverse engineering & protocol analysis
├── Fridge Snoops/          # 🔍  Fridge BLE protocol captures & decoding
├── Check List/             # ✅  UI testing screenshots & mockups
├── Examples/               # 📚  Code references & library examples
```

### **🗂️ Configuration & Docs**
```
├── .claude/                # 🤖  AI assistant project configuration
├── .git/                   # 📝  Version control
├── Backups/                # 💾  Project backups (consolidated)
├── Device MAC & Keys.txt   # 🔑  Hardware identifiers
├── *.md                    # 📋  Project documentation
```

---

## 🌐 System Architecture

### **Communication Flow**
```
┌─────────────────┐    ESP-NOW    ┌──────────────────┐
│   Victron_ESP32 │ ◄────────────► │   Master_ESP32   │
│   (BLE Scanner) │                │  (Web Dashboard) │
└─────────────────┘                └──────────────────┘
                                            ▲
┌─────────────────┐    ESP-NOW             │
│  EcoFlow_ESP32  │ ◄──────────────────────┤
│   (BLE Scanner) │                        │
└─────────────────┘                        │ ESP-NOW
                                           │
┌─────────────────┐    ESP-NOW             ▼
│   Fridge_ESP32  │ ◄──────────────────────┤
│ (BLE Controller)│                        │
└─────────────────┘                        │
                                           │
┌─────────────────┐    WiFi AP             │
│  iPad/Phone     │ ◄──────────────────────┘
│ (Web Interface) │    192.168.4.1
└─────────────────┘
```

### **Device Details**
| Device | MAC Address | Function | Protocol |
|--------|-------------|----------|----------|
| **Master** | `7C:87:CE:31:FE:50` | Web dashboard, ESP-NOW coordinator | WiFi AP + ESP-NOW |
| **Victron** | `78:21:84:9C:9B:88` | Solar system data collection | BLE scan → ESP-NOW |
| **EcoFlow** | *(Dynamic)* | Battery pack monitoring | BLE scan → ESP-NOW |
| **Fridge** | *(Dynamic)* | Temperature control | BLE connect ← ESP-NOW |

---

## 🎨 Web Interface

### **Page Structure**
```
📱 Main Dashboard (/)
├── 🎛️ System Monitor Cards (Solar, Battery, Fridge)
├── 🧭 Navigation (Fridge, Inventory)
└── 🏠 Home to Monitoring

📋 Inventory System (/inventory)
├── Tab 0: 🏠 Main Dashboard (Card layout)
├── Tab 1: 🍽️ Consumables (Status tracking)
├── Tab 2: 🚚 Trailer Equipment (Packed/Checked)
├── Tab 3: ⭐ Essential Equipment (Must-pack items)
├── Tab 4: 📦 Optional Equipment (Trip-dependent)
├── Tab 5: 🛒 Shopping List (Auto-generated)
└── 💾 Backups (/inventory/backups)

❄️ Fridge Control (/fridge)
├── 🌡️ Temperature zones (Left/Right)
├── 🎚️ Quick adjust buttons (-5/-1/+1/+5)
├── 🍃 ECO mode toggle
├── 🔋 Battery protection (L/M/H)
└── 🧭 Navigation (Main Dashboard, Inventory)
```

### **Navigation Flow**
```
Monitor (/) ◄─── Main Dashboard (/inventory) ◄─── All Inventory Tabs
                         ▲                            ├── Consumables
                         │                            ├── Trailer
                         └─── Fridge Control (/fridge) ├── Essentials
                                                       ├── Optional
                                                       ├── Shopping
                                                       └── Backups
```

---

## 💾 Data Management

### **Inventory Structure**
```cpp
struct InventoryCategory {
    String name;              // "🍽️ Food & Cooking"
    String icon;              // "🍽️"
    bool isConsumable;        // true = has status (OK/Low/Out)
    SubCategory subcategory;  // TRAILER/ESSENTIALS/OPTIONAL
    std::vector<ConsumableItem> consumables;  // For consumables
    std::vector<EquipmentItem> equipment;     // For equipment
};

// 3-Level Equipment Structure:
// 🚚 TRAILER (4 categories)    - Always in trailer
// ⭐ ESSENTIALS (3 categories) - Must pack every trip  
// 📦 OPTIONAL (5 categories)   - Trip-dependent
```

### **Smart Backup System**
```cpp
// Professional retention policy
Daily backups:    7 days    (backup_daily_YYYYMMDD.json)
Weekly backups:   4 weeks   (backup_weekly_YYYYMMDD.json)  
Monthly backups:  6 months  (backup_monthly_YYYYMM.json)
Quarterly backups: 4 quarters (backup_quarterly_YYYYQQ.json)

Total storage: ~158KB (vs 1.28MB available = 12% usage)
```

---

## 🔧 Hardware Configuration

### **ESP32 Connections**
```
Master ESP32 (Main Hub):
├── Power: USB or 5V supply
├── WiFi: Access Point mode (SSID: PowerMonitor)
├── ESP-NOW: Coordinator for all devices
└── SPIFFS: 1.28MB for backups & config

Peripheral ESP32s:
├── Victron_ESP32: BLE scanning (BMV-712, MPPT, IP22)
├── EcoFlow_ESP32: BLE scanning (Delta 2 Max)  
├── Fridge_ESP32: BLE connection (Flex Adventure 95L)
└── All: ESP-NOW communication to Master
```

### **Future Expansion Ready**
```
📐 MPU-6050 Leveling System (Planned):
├── Hardware: I2C connection to Master ESP32
├── Software: /level page with visual bubble level
└── Integration: Added to main dashboard navigation
```

---

## 🚀 Development Status

### **✅ Completed Features**
- [x] ESP-NOW bidirectional communication (100% working)
- [x] BLE protocol reverse engineering (Victron, EcoFlow, Fridge)
- [x] Web dashboard with real-time monitoring
- [x] Dynamic inventory system with categories
- [x] Smart backup/restore system
- [x] Fridge temperature control via web interface  
- [x] Shopping list generation from consumables
- [x] iPad-optimized UI with consistent styling
- [x] Professional navigation flow
- [x] Card-based dashboard layout

### **🎯 Next Milestones**
- [ ] MPU-6050 leveling system integration
- [ ] Additional sensors (ambient temp, Master battery level)
- [ ] Mobile app icon & PWA features

### **📊 Performance Metrics**
```
Flash Usage: 90.2% (1,182,762 bytes)
RAM Usage: 13.9% (45,472 bytes)
SPIFFS Usage: 12% (158KB backups)
ESP-NOW Success Rate: 100%
Web Response Time: <500ms
```

---

## 🔄 Maintenance

### **Regular Tasks**
- Monitor flash usage (currently at 90.2% - near limit)
- Check backup system logs
- Verify ESP-NOW communication health
- Update device firmware as needed

### **Troubleshooting**
- ESP-NOW issues: Check MAC addresses in `Device MAC & Keys.txt`
- BLE connection problems: Review protocol docs in respective folders
- Web interface issues: Check console logs and device serial output
- Backup failures: Verify SPIFFS space and file permissions

---

## 📝 Documentation Index

| Document | Purpose |
|----------|---------|
| `PROJECT_STRUCTURE.md` | This file - complete project overview |
| `INVENTORY_SYSTEM.md` | Detailed inventory system documentation |
| `FRIDGE_BLE_PROTOCOL.md` | Fridge control protocol specifications |
| `ESP_NOW_LESSONS_LEARNED.md` | ESP-NOW implementation notes |
| `TODO_TOMORROW.md` | Current tasks and priorities |
| `PROJECT_STATUS.md` | Historical development status |
| `CLEANUP_PLAN.md` | Project organization guidelines |

---

**🎉 Project Status: Production Ready**  
*Successfully monitoring solar system, battery, and fridge with full web control interface*