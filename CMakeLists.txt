cmake_minimum_required(VERSION 3.10)
project(WeatherDisplay VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output binary to build/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Includes
include_directories(${PROJECT_SOURCE_DIR}/include)

# Sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/*.h"
)

# Create executable
add_executable(weather_display ${SOURCES} ${HEADERS})

# Link required libraries
target_link_libraries(weather_display wiringPi curl)

# Install binary
install(TARGETS weather_display RUNTIME DESTINATION /usr/local/bin)

# Install systemd service file
install(FILES ${PROJECT_SOURCE_DIR}/systemd/weather-display.service
        DESTINATION /etc/systemd/system
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

# Optional: install config file if needed
# install(FILES config.json DESTINATION /etc/weather_display)

# Add custom target to enable + start systemd service
add_custom_target(install-service
    COMMAND ${CMAKE_COMMAND} -E echo "Installing and enabling systemd service..."
    COMMAND sudo ${CMAKE_COMMAND} --install . --prefix /usr/local
    COMMAND sudo systemctl daemon-reexec
    COMMAND sudo systemctl daemon-reload
    COMMAND sudo systemctl enable weather-display.service
    COMMAND sudo systemctl restart weather-display.service
    COMMENT "Installing and enabling WeatherDisplay as a systemd service"
)
