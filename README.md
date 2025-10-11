# WeatherDisplay

# Compile
# podman build -t weather-display .

# Temp Container
# podman create --name weather-temp weather-display

# Copy Binary out of Container
# podman cp weather-temp:/app/weather ./weather

# Destroy Temp Container
# podman rm weather-temp

# Make Binary Executable
# chmod +x ./weather

# Run Bin
# ./weather

