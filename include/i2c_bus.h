#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <cstdint>
#include <cstddef>

// Thin I2C abstraction so the same code compiles on Pi (real I2C via wiringPi)
// and on Mac/Linux dev machines (mock that records writes for testing).
//
// Build with -DI2C_REAL on the Pi. Omit it to build the mock for Mac.
class I2CBus {
public:
    bool open(int address);
    void close();

    bool writeByte(uint8_t value);
    bool writeCommand(uint8_t cmd);                          // [0x00, cmd]
    bool writeData(const uint8_t* data, std::size_t len);    // [0x40, data...]

    bool isOpen()  const { return fd_ >= 0; }
    int  address() const { return address_; }

#ifndef I2C_REAL
    // Mock-only inspection helpers for tests.
    static void        mockReset();
    static std::size_t mockCommandCount();
    static std::size_t mockDataByteCount();
#endif

private:
    int fd_      = -1;
    int address_ = -1;
};

#endif