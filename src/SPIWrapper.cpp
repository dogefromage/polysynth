#include "SPIWrapper.h"

SPIWrapperSettings::SPIWrapperSettings(uint32_t clk, uint8_t order, uint8_t mode)
    : clock(clk), bitOrder(order), dataMode(mode) {}

SPIWrapper::SPIWrapper(uint32_t bitbangThreshold, uint8_t mosiPin, uint8_t sckPin)
    : bitbang_threshold(bitbangThreshold), mosi_pin(mosiPin), sck_pin(sckPin), use_bitbang(false), current_settings(4000000, MSBFIRST, SPI_MODE0) {}

void SPIWrapper::beginTransaction(const SPIWrapperSettings& settings) {
    use_bitbang = settings.clock < bitbang_threshold;
    current_settings = settings;

    if (use_bitbang) {
        pinMode(mosi_pin, OUTPUT);
        pinMode(sck_pin, OUTPUT);
        digitalWrite(mosi_pin, LOW);
        digitalWrite(sck_pin, (cpol() ? HIGH : LOW));
    } else {
        SPI.beginTransaction(SPISettings(settings.clock, settings.bitOrder, settings.dataMode));
    }
}

void SPIWrapper::endTransaction() {
    if (!use_bitbang) {
        SPI.endTransaction();
    }
}

void SPIWrapper::transfer16(uint16_t data) {
    if (use_bitbang) {
        bitbangTransfer16(data);
    } else {
        SPI.transfer16(data);
    }
}

bool SPIWrapper::cpol() const {
    return (current_settings.dataMode & 0x02);
}

bool SPIWrapper::cpha() const {
    return (current_settings.dataMode & 0x01);
}

void SPIWrapper::delayBit() const {
    delayMicroseconds(1);  // tweak for speed
}

void SPIWrapper::bitbangTransfer16(uint16_t data) {
    for (int i = 0; i < 16; ++i) {
        uint8_t bit_index = (current_settings.bitOrder == MSBFIRST) ? 15 - i : i;
        bool bit = (data >> bit_index) & 1;

        digitalWrite(mosi_pin, bit);

        if (!cpha()) {
            toggleClock();
        } else {
            delayBit();
            toggleClock();
        }
    }

    // restore clock to idle
    digitalWrite(sck_pin, cpol() ? HIGH : LOW);
}

void SPIWrapper::toggleClock() {
    bool idle = cpol();
    bool active = !idle;

    digitalWrite(sck_pin, active);
    delayBit();
    digitalWrite(sck_pin, idle);
    delayBit();
}
