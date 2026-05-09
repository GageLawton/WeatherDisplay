# Mac-only test build for the RingDisplay controller (mock SPI).
#   make -f Makefile.ring
#   ./test_ring
#   open test_output/ring_day_*.ppm

CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
SRCS     := src/celestial.cpp src/neopixel.cpp src/ring.cpp test/test_ring.cpp
TARGET   := test_ring

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

view: $(TARGET)
	./$(TARGET)
	@open test_output/ring_day_*.ppm

clean:
	rm -f $(TARGET)

.PHONY: all clean view