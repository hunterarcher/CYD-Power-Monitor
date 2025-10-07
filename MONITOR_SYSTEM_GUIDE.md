# Smart Alert & Monitor System Guide
**Version:** 1.0  
**Date:** October 7, 2025  
**Status:** Production Ready ✅

## 🚨 Smart Alert System

### Overview
Intelligent notification bar that slides down from the top of the dashboard to provide instant system status feedback.

### Alert Types & Behavior

#### ✅ Normal Status
- **Message:** "✓ All Systems Normal"
- **Color:** Green gradient background
- **Behavior:** Shows for 3 seconds on navigation only (not refresh), then disappears
- **Trigger:** Memory >60KB, battery >20%, data connection active

#### ⚡ Caution Level  
- **Message:** "⚡ Caution: Monitor Memory (XXkb) - Click for Details"
- **Color:** Yellow/amber gradient
- **Behavior:** Stays visible until resolved, clickable to jump to Monitor page
- **Trigger:** Memory 40-60KB

#### ⚠️ Warning Level
- **Message:** "⚠ Warning: Memory Low (XXkb)" or "🔋 Warning: Battery XX%"
- **Color:** Orange gradient  
- **Behavior:** Persistent, clickable, requires attention
- **Trigger:** Memory 20-40KB OR battery 10-20%

#### 🔴 Critical Level
- **Message:** "⚠ Critical: Low Memory" or "🔋 Critical: Battery XX%" or "⚠ Data Connection Lost"
- **Color:** Red gradient with pulsing animation
- **Behavior:** Persistent, pulsing, clickable, immediate attention required
- **Trigger:** Memory <20KB OR battery <10% OR no data for 60+ seconds

### Smart Navigation Detection
```javascript
// Only show "All Systems Normal" on navigation, not refresh
let isNavigation = !document.referrer || 
                   document.referrer.indexOf(window.location.origin) == -1 ||
                   document.referrer.indexOf('/monitor') > -1 || 
                   document.referrer.indexOf('/fridge') > -1 || 
                   document.referrer.indexOf('/inventory') > -1;
```

## 📊 Monitor Page (/monitor)

### Real-Time System Metrics

#### Memory Monitoring Card
- **Free Memory:** Current available heap in KB with color-coded alert level
- **Memory Usage:** Percentage bar with gradient visualization  
- **Alert Status:** Visual indicator (Normal/Caution/Warning/Critical)
- **Threshold Details:** Displays current alert level and memory amount

#### System Information Card
- **CPU Frequency:** Current ESP32 clock speed in MHz
- **Uptime:** System runtime in hours and minutes
- **Data Status:** Active/Stale indicator with last update time
- **Packet Count:** Total ESP-NOW packets received

#### Hardware Details
- **Chip Revision:** ESP32 hardware version
- **Max Alloc Heap:** Maximum single allocation possible
- **Free Flash Space:** Available program storage
- **Alert Level:** Current system health status

### Memory Alert Thresholds
```cpp
String alertLevel = "Normal";     // >60KB - Green
if (freeHeap < 60000) alertLevel = "Caution";   // 40-60KB - Yellow  
if (freeHeap < 40000) alertLevel = "Warning";   // 20-40KB - Orange
if (freeHeap < 20000) alertLevel = "Critical";  // <20KB - Red
```

### Auto-Refresh System
- **Automatic:** Page refreshes every 5 seconds
- **Manual:** Click refresh button (↻) for immediate update
- **No Alert Spam:** Refresh doesn't trigger "All Systems Normal" alert

## 🎯 Navigation System

### Clean Three-Button Layout
- **Dashboard** (🏠): Return to main power monitoring page
- **Fridge** (🧊): Access fridge control and temperature settings  
- **Inventory** (🚚): Manage camping supplies and shopping lists

### Navigation Improvements
- **Removed:** Confusing highlighted "Monitor" button on Monitor page
- **Removed:** Redundant "← Dashboard" back button at top
- **Added:** Consistent navigation buttons at bottom of all pages
- **Responsive:** Adapts to mobile and tablet screen sizes

## 🛠️ Technical Implementation

### Alert Bar CSS Classes
```css
.alert-bar.normal    { background: linear-gradient(135deg,#22c55e,#16a34a); }
.alert-bar.caution   { background: linear-gradient(135deg,#eab308,#ca8a04); }
.alert-bar.warning   { background: linear-gradient(135deg,#f97316,#ea580c); }
.alert-bar.critical  { background: linear-gradient(135deg,#ef4444,#dc2626); }
```

### Monitor Page Styling
- **Cards:** Same gradient backgrounds as main dashboard
- **Typography:** Consistent fonts and sizing across all pages
- **Colors:** Color-coded values (voltage=blue, current=white, power=orange, temp=cyan)
- **Responsive:** Grid layouts that adapt to screen size

### Integration Points
- **Memory Monitoring:** `ESP.getFreeHeap()` checked every page load
- **Battery Health:** Integrated with Victron BMV-712 SOC data
- **Data Connectivity:** Monitors ESP-NOW packet timestamps
- **Click Actions:** Alert bar clicks jump to Monitor page for details

## 🚀 Usage Guide

### For Users
1. **Normal Operation:** Green "All Systems Normal" briefly appears when navigating to dashboard
2. **Monitoring:** Click "Monitor" button to see detailed system health
3. **Alerts:** If colored alert appears, click it for detailed diagnostics
4. **Navigation:** Use bottom buttons to move between Dashboard/Fridge/Inventory

### For Developers  
1. **Memory Management:** Monitor free heap to prevent crashes
2. **Alert Thresholds:** Adjust memory limits based on operational requirements
3. **Battery Integration:** Expand battery alerts for other power sources
4. **Custom Alerts:** Add new alert types in `window.onload` function

## 📈 Future Enhancements

### Planned Features
- **1AM Auto-Restart:** Scheduled daily restart for memory cleanup
- **Enhanced Cleanup:** Automated maintenance routines
- **Alert History:** Log of past alerts and system events
- **Email Notifications:** Send alerts to external monitoring systems
- **Performance Trends:** Historical memory and performance data

### Monitoring Expansion
- **Temperature Alerts:** ESP32 internal temperature monitoring
- **WiFi Signal:** Connection quality alerts
- **Flash Wear:** Storage health monitoring
- **Power Consumption:** Battery life predictions

---
**System Status:** ✅ Production Ready  
**Next Steps:** 1AM Auto-Restart Implementation  
**Documentation Updated:** October 7, 2025