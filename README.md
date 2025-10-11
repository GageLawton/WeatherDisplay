# WeatherDisplay

A lightweight C++ application for displaying real-time weather on an LCD, running on a Raspberry Pi 5. Built using Podman and deployed as a persistent systemd service.

---

## 🔧 Installation Instructions

### 1. Clone the repository
git clone https://github.com/GageLawton/WeatherDisplay.git
cd WeatherDisplay

### 2. Configure your API key and location
Edit main.cpp and update the following lines with your OpenWeatherMap API key and desired location:
const std::string API_KEY = "your-api-key";
const std::string LOCATION = "Westmont, IL";  // or your preferred city

### 3. Build and Run (One-time Test Mode)
To build the project and run it manually (not as a service):
chmod +x build-and-run.sh
./build-and-run.sh

### 4. ✅ Run as a Persistent Service (Recommended)
To build the app, install it as a background service, and enable it on boot:
chmod +x build-and-install-service.sh
./build-and-install-service.sh

### 5. ✅ Verify the Service is Running
Check service status:
sudo systemctl status weather-display.service

### (Optional) Stream live logs:
journalctl -u weather-display.service -f

### (Optional) Reboot Test
Reboot test:
sudo reboot
# Then check again
systemctl status weather-display.service

### (if needed) Uninstall 
sudo systemctl stop weather-display.service
sudo systemctl disable weather-display.service
sudo rm /etc/systemd/system/weather-display.service
sudo systemctl daemon-reload

### Project Strcuture
.
├── Dockerfile
├── main.cpp
├── lcd.cpp / lcd.h
├── weather.cpp / weather.h
├── build-and-run.sh
├── build-and-install-service.sh
├── install-service.sh
├── systemd/
│   └── weather-display.service
└── README.md

### Dependencies
Raspberry Pi OS
Podman (or Docker, with small modifications)
OpenWeatherMap API Key
I2C LCD Display (1602 or similar)

### Release Version
v1.0.0.0