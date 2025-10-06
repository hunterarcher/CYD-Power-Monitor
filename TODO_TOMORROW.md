# TODO List - 2025-10-06

## 🧪 Testing & Validation
- [ ] Test all pages on iPad in landscape mode
- [ ] Verify full-screen mode works (or accept limitation)
- [ ] Test fridge page quick adjust buttons (+1, +5, -1, -5)
- [ ] Test inventory add/remove items
- [ ] Test shopping list copy/download features
- [ ] Check for any remaining mobile popup issues

## 🧹 Project Cleanup
- [ ] Review **CLEANUP_PLAN.md**
- [ ] Check **Archive/** folder contents before deleting
- [ ] Consolidate **Backups/** folder (keep only recent)
- [ ] Delete old test folders (confirm first)
- [ ] Delete large log files (~16MB btsnoop files)
- [ ] Delete old screenshots (moved to Check List)
- [ ] Delete outdated documentation files
- [ ] Create **PROJECT_STRUCTURE.md** documenting new structure
- [ ] Update **.gitignore** to prevent future clutter

## 📐 MPU-6050 Leveling System (New Feature)

### Research Phase
- [ ] Review MPU-6050 specifications
- [ ] Check I2C pin availability on Master ESP32
- [ ] Research ESP32 MPU-6050 libraries (Adafruit? MPU6050_light?)
- [ ] Understand calibration requirements
- [ ] Review pitch/roll calculations

### Hardware Planning
- [ ] Order MPU-6050 module (if not available)
- [ ] Plan wiring to Master ESP32:
  - VCC → 3.3V
  - GND → GND
  - SDA → GPIO 21 (default I2C)
  - SCL → GPIO 22 (default I2C)
- [ ] Consider mounting location on trailer
  - Rigid mounting point
  - Protected from weather
  - Accessible for calibration
  - Account for trailer flex

### Software Design
- [ ] Design UI for leveling page (`/level`)
  - Visual bubble level graphic
  - Real-time pitch/roll angles
  - Color-coded indicators (green=level, yellow/red=tilted)
  - Front/Back tilt indicator
  - Left/Right tilt indicator
  - Calibrate button
- [ ] Plan data structure for MPU-6050 readings
- [ ] Plan calibration workflow
  - Save "zero" position to SPIFFS
  - Reset calibration option
  - Calibration instructions on page

### Implementation Tasks
- [ ] Add MPU-6050 library to Master_ESP32/platformio.ini
- [ ] Create I2C initialization code
- [ ] Implement sensor reading in main loop
- [ ] Create `/level` web page endpoint
- [ ] Design bubble level SVG/CSS graphics
- [ ] Add live updates (AJAX polling or WebSocket?)
- [ ] Implement calibration save/load to SPIFFS
- [ ] Add link to leveling page on main dashboard
- [ ] Test with actual sensor mounted

## 📚 Documentation Updates
- [ ] Update **INVENTORY_SYSTEM.md** with latest changes
- [ ] Create **LEVELING_SYSTEM.md** (once implemented)
- [ ] Update README (if exists) or create one
- [ ] Document MPU-6050 wiring diagram
- [ ] Add calibration procedure to docs

## 🐛 Known Issues to Address
- [ ] Full-screen navigation on iOS (accept limitation or find workaround)
- [ ] Any other issues discovered during testing

## 💡 Nice-to-Have Ideas (Backlog)
- [ ] Add battery level indicator for Master ESP32 itself
- [ ] Add temperature sensor to Master (ambient temp)
- [ ] Create settings page for WiFi credentials
- [ ] Add ability to export/import inventory as JSON file
- [ ] Night mode toggle for dashboard
- [ ] Add sound alerts for critical battery levels
- [ ] Create mobile app icon for home screen

## 📊 Current System Status

### ✅ Completed (as of 2025-10-05)
- Dynamic inventory system with add/remove items
- Shopping list with selective restocking
- Color-coded fridge temperature monitoring
- ESP-NOW communication between all devices
- iPad-optimized dashboard layout
- Compact fridge control page with quick adjust buttons
- Disabled annoying mobile lookup popups
- All navigation working properly

### 🔧 Active Devices
1. **Master ESP32** - Central hub with web dashboard
2. **Victron ESP32** - Solar/battery monitoring (BLE)
3. **EcoFlow ESP32** - Battery pack monitoring (BLE)
4. **Fridge ESP32** - Fridge control (BLE)

### 📈 Next Major Milestone
MPU-6050 leveling system integration

---

## Notes
- Keep sessions focused - one major task per session
- Test thoroughly before moving to next feature
- Document as you go
- Commit frequently with good messages
