#include "i2c_bus.h"
#include <cstdio>
#include <vector>

#if defined(I2C_REAL)
// ---- Real Pi implementation via wiringPi ----
#include <wiringPiI2C.h>
#include <unistd.h>

bool I2CBus::open(int address) {
    int fd = wiringPiI2CSetup(address);
    if (fd < 0) return false;
    fd_ = fd;
    address_ = address;
    return true;
}

void I2CBus::close() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

bool I2CBus::writeByte(uint8_t value) {
    if (fd_ < 0) return false;
    return wiringPiI2CWrite(fd_, value) >= 0;
}

bool I2CBus::writeCommand(uint8_t cmd) {
    if (fd_ < 0) return false;
    // SSD1306 control byte: Co=0, D/C#=0 -> 0x00, then command byte.
    return wiringPiI2CWriteReg8(fd_, 0x00, cmd) >= 0;
}

bool I2CBus::writeData(const uint8_t* data, std::size_t len) {
    if (fd_ < 0 || data == nullptr) return false;
    // SSD1306 data: 0x40 control byte then bulk pixel bytes.
    // Use raw write() for the bulk transfer; faster than per-byte calls.
    std::vector<uint8_t> buf;
    buf.reserve(len + 1);
    buf.push_back(0x40);
    for (std::size_t i = 0; i < len; ++i) buf.push_back(data[i]);
    ssize_t n = ::write(fd_, buf.data(), buf.size());
    return n == static_cast<ssize_t>(buf.size());
}

#else
// ---- Mock implementation for Mac development ----
namespace {
    std::size_t g_cmd_count       = 0;
    std::size_t g_data_byte_count = 0;
}

bool I2CBus::open(int address) { fd_ = 1; address_ = address; return true; }
void I2CBus::close()           { fd_ = -1; }
bool I2CBus::writeByte(uint8_t)  { return fd_ >= 0; }

bool I2CBus::writeCommand(uint8_t) {
    if (fd_ < 0) return false;
    g_cmd_count++;
    return true;
}

bool I2CBus::writeData(const uint8_t* data, std::size_t len) {
    if (fd_ < 0 || data == nullptr) return false;
    g_data_byte_count += len;
    return true;
}

void        I2CBus::mockReset()         { g_cmd_count = 0; g_data_byte_count = 0; }
std::size_t I2CBus::mockCommandCount()  { return g_cmd_count; }
std::size_t I2CBus::mockDataByteCount() { return g_data_byte_count; }
#endif