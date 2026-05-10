# Mac-only ring controller test (mock SPI).
#   make -f test/Makefile.ring
#   ./test_ring
#   open test_output/ring_day_*.ppm

CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
SRCS     := src/celestial.cpp src/neopixel.cpp src/ring.cpp test/test_ring.cpp
TARGET   := test_ring

all: $(TARGET)

view: $(TARGET)
	./$(TARGET)
	@open test_output/ring_day_*.ppm

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean view