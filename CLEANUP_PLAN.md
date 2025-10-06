# Project Cleanup Plan

## Date: 2025-10-05

## 📁 Folders to Keep (Active Projects)

### Core Projects
- **Master_ESP32/** - ✅ MAIN PROJECT - ESP-NOW coordinator with web dashboard
- **Victron_ESP32/** - ✅ ACTIVE - Solar monitoring via BLE
- **EcoFlow_ESP32/** - ✅ ACTIVE - Battery monitoring via BLE
- **Fridge_ESP32/** - ✅ ACTIVE - Fridge control via BLE

### Important Folders
- **.git/** - Version control
- **.claude/** - AI assistant configuration
- **Backups/** - Project backups (review and consolidate)
- **Check List/** - Screenshots and testing materials

## 🗑️ Folders to DELETE

### Old/Deprecated Projects
- **Master_ESP32_V2/** - Superseded by Master_ESP32
- **Victron_ESP32_V2/** - Superseded by Victron_ESP32
- **Master_WiFi_Sniffer/** - Old sniffer attempt
- **Master_WiFi_Sniffer_Clean/** - Old sniffer attempt
- **WiFi_Sniffer_ESP32/** - Old sniffer attempt

### Test Folders (No longer needed)
- **Test/** - Generic test folder
- **New Test/** - Generic test folder
- **Test_ESP_NOW_Bidirectional/** - ESP-NOW tests (knowledge captured in docs)
- **Test_ESP_NOW_Master/** - ESP-NOW tests (knowledge captured in docs)
- **Test_ESP_NOW_Slave/** - ESP-NOW tests (knowledge captured in docs)

### Library/Reference (Can be re-cloned if needed)
- **victron-ble/** - GitHub library (can re-clone)
- **victron-hacs/** - GitHub library (can re-clone)

### Weird/Broken
- **Backups$(date +%Y%m%d_%H%M%S)_PreFridgeIntegration/** - Malformed folder name
- **Archive/** - Check contents first, likely old stuff

## 📄 Files to DELETE

### Log Files (Outdated)
- **btsnoop_hci.log** (7.5 MB) - Old BLE capture
- **btsnoop_hci.log.last** (8.8 MB) - Old BLE capture
- **Export btsnoop.txt** (397 KB) - Old export
- **logs.txt** (34 KB) - Old logs
- **Log 2025-10-01 12_07_10.txt** (13 KB) - Old log

### Screenshots (Moved to Check List or redundant)
- **app data.jpeg** (duplicate of Check List/app data.jpeg?)
- **Main Pgae Example.jpeg** (moved to Check List?)
- **Main screen now.jpeg** (moved to Check List?)
- **odd screen.jpeg** - Old screenshot
- **Screenshot 2025-09-30 204835.png** - Old screenshot
- **WhatsApp Image 2025-10-01 at 13.14.41 (1).jpeg** - Old screenshot
- **WhatsApp Image 2025-10-01 at 13.14.41.jpeg** - Old screenshot

### Old Documentation (Outdated or superseded)
- **TOMORROW_START_HERE.md** - Old session notes
- **SESSION_SUMMARY.md** - Old session summary
- **PROJECT_STATUS.md** - Outdated status (now in git commits)

### Misc Files
- **nul** - Empty/error file
- **fridge_connection_comparison.txt** - Old comparison notes
- **fridge_sniffer.txt** - Old sniffer output

## 📄 Files to KEEP

### Active Documentation
- **INVENTORY_SYSTEM.md** - ✅ Current inventory system docs
- **FRIDGE_BLE_PROTOCOL.md** - ✅ Fridge protocol reference
- **ESP_NOW_LESSONS_LEARNED.md** - ✅ Important ESP-NOW knowledge
- **ESP-NOW_TROUBLESHOOTING.md** - ✅ Troubleshooting guide
- **INTEGRATION_GUIDE.md** - ✅ Integration documentation
- **ANALYZE_BTSNOOP.md** - ✅ BLE analysis guide
- **fridge_knowledge.md** - ✅ Fridge knowledge base
- **flex_ble_summary.md** - ✅ BLE summary

### Configuration
- **.gitignore** - Git configuration
- **Trailer Build.code-workspace** - VS Code workspace
- **Device MAC & Keys.txt** - Device credentials

### Scripts
- **scan_all_ports.ps1** - Port scanning utility
- **scan_ecoflow.ps1** - EcoFlow scanning
- **test_ecoflow_port.ps1** - EcoFlow testing

### Data Folders
- **EcoFlow Data/** - Keep for reference
- **Fridge Snoops/** - Keep BLE captures
- **Examples/** - Keep examples

## 📊 Estimated Space Savings

- **Folders to delete**: ~10-15 test/old project folders
- **Large files to delete**: ~16.5 MB (btsnoop logs)
- **Screenshots to delete**: ~400 KB
- **Total estimated savings**: ~17-20 MB + cleaner structure

## ✅ Actions for Tomorrow

1. **Review Archive/ folder** - Check if anything important before deleting
2. **Consolidate Backups/** - Keep only recent/important backups
3. **Execute deletions** (with confirmation)
4. **Create PROJECT_STRUCTURE.md** - Document new clean structure
5. **Update .gitignore** - Prevent future clutter

## 📝 Notes

- All test code knowledge has been captured in documentation
- Git history preserves all old code if needed
- BLE protocol knowledge documented in FRIDGE_BLE_PROTOCOL.md
- Can always re-clone libraries from GitHub
