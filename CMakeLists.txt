cmake_minimum_required(VERSION 3.10)
project(WeatherDisplay VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/include)

# All sources (config, lcd, weather, oled, font, i2c_bus, neopixel, ring,
# celestial, main)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.h")

# Exclude Mac-only mocks from the Pi/CI build.
list(REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/src/lcd_mock.cpp")

add_executable(weather_display ${SOURCES} ${HEADERS})

# I2C: real wiringPi backend.
target_compile_definitions(weather_display PRIVATE I2C_REAL)

# NeoPixel: real Linux SPI backend.
target_compile_definitions(weather_display PRIVATE NEOPIXEL_REAL)

target_link_libraries(weather_display wiringPi curl)

install(TARGETS weather_display RUNTIME DESTINATION /usr/local/bin)

install(FILES ${PROJECT_SOURCE_DIR}/systemd/weather-display.service
        DESTINATION /etc/systemd/system
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

add_custom_target(install-service
    COMMAND ${CMAKE_COMMAND} -E echo "Installing and enabling systemd service..."
    COMMAND sudo ${CMAKE_COMMAND} --install . --prefix /usr/local
    COMMAND sudo systemctl daemon-reload
    COMMAND sudo systemctl enable weather-display.service
    COMMAND sudo systemctl restart weather-display.service
    COMMENT "Installing and enabling WeatherDisplay as a systemd service"
)