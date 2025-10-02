# ESP32 Trailer Monitor System - PowerShell Helper Functions
# Place this in your PowerShell profile or run it in your session

# Navigate to project directory
function Go-TrailerMonitor {
    Set-Location "C:\Users\LENOVO\OneDrive\Documents\PlatformIO\Projects\Trailer_Monitor_System"
    Write-Host "📍 Switched to Trailer Monitor System project" -ForegroundColor Green
}

# Build the project
function Build-TrailerMonitor {
    Write-Host "🔨 Building ESP32 Trailer Monitor System..." -ForegroundColor Yellow
    python -m platformio run
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Build successful!" -ForegroundColor Green
    } else {
        Write-Host "❌ Build failed!" -ForegroundColor Red
    }
}

# Upload firmware to ESP32
function Upload-TrailerMonitor {
    Write-Host "📤 Uploading to ESP32..." -ForegroundColor Yellow
    python -m platformio run --target upload
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Upload successful!" -ForegroundColor Green
    } else {
        Write-Host "❌ Upload failed!" -ForegroundColor Red
    }
}

# Monitor serial output
function Monitor-TrailerSystem {
    Write-Host "📺 Starting serial monitor (Ctrl+C to stop)..." -ForegroundColor Yellow
    Write-Host "Looking for Victron BLE devices..." -ForegroundColor Cyan
    python -m platformio device monitor --baud 115200
}

# Build and upload in one command
function Deploy-TrailerMonitor {
    Write-Host "🚀 Building and uploading ESP32 Trailer Monitor System..." -ForegroundColor Yellow
    python -m platformio run --target upload
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Deploy successful!" -ForegroundColor Green
        Write-Host "Starting monitor in 3 seconds..." -ForegroundColor Cyan
        Start-Sleep -Seconds 3
        Monitor-TrailerSystem
    } else {
        Write-Host "❌ Deploy failed!" -ForegroundColor Red
    }
}

# Clean build files
function Clean-TrailerMonitor {
    Write-Host "🧹 Cleaning build files..." -ForegroundColor Yellow
    python -m platformio run --target clean
    Write-Host "✅ Clean complete!" -ForegroundColor Green
}

# Show available COM ports
function Show-ComPorts {
    Write-Host "🔌 Available COM ports:" -ForegroundColor Yellow
    python -m platformio device list
}

# Quick status check
function Status-TrailerMonitor {
    Write-Host "📊 ESP32 Trailer Monitor System Status" -ForegroundColor Cyan
    Write-Host "======================================" -ForegroundColor Cyan
    
    # Check if we're in the right directory
    if (Test-Path "platformio.ini") {
        Write-Host "✅ Project directory: OK" -ForegroundColor Green
    } else {
        Write-Host "❌ Not in project directory" -ForegroundColor Red
        return
    }
    
    # Check build status
    if (Test-Path ".pio\build\mhetesp32devkit\firmware.bin") {
        Write-Host "✅ Firmware built: OK" -ForegroundColor Green
    } else {
        Write-Host "⚠️  Firmware not built" -ForegroundColor Yellow
    }
    
    # Show configured devices
    Write-Host "📡 Configured Victron devices:" -ForegroundColor White
    Write-Host "  • KARSTEN MAXI SHUNT (c0:3b:98:39:e6:fe)" -ForegroundColor Gray
    Write-Host "  • KARSTEN MAXI SOLAR (e8:86:01:5d:79:38)" -ForegroundColor Gray  
    Write-Host "  • KARSTEN MAXI AC (c7:a2:c2:61:9f:c4)" -ForegroundColor Gray
    
    Write-Host "`n🔧 Available commands:" -ForegroundColor White
    Write-Host "  • Build-TrailerMonitor     - Build firmware" -ForegroundColor Gray
    Write-Host "  • Upload-TrailerMonitor    - Upload to ESP32" -ForegroundColor Gray
    Write-Host "  • Monitor-TrailerSystem    - Watch serial output" -ForegroundColor Gray
    Write-Host "  • Deploy-TrailerMonitor    - Build + Upload + Monitor" -ForegroundColor Gray
    Write-Host "  • Show-ComPorts            - List COM ports" -ForegroundColor Gray
}

# Display welcome message
Write-Host ""
Write-Host "🔋 ESP32 Trailer Monitor System - PowerShell Tools Loaded!" -ForegroundColor Green
Write-Host "Run 'Status-TrailerMonitor' to see available commands" -ForegroundColor Cyan
Write-Host ""