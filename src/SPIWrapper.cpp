#include "SPIWrapper.h"

SPIWrapperSettings::SPIWrapperSettings(uint32_t clk, uint8_t order, uint8_t mode, uint8_t mosiPin, uint8_t sckPin)
    : clock(clk), bitOrder(order), dataMode(mode), mosi_pin(mosiPin), sck_pin(sckPin) {}

SPIWrapper::SPIWrapper(uint32_t bitbangThreshold)
    : bitbang_threshold(bitbangThreshold), use_bitbang(false), current_settings(4000000, MSBFIRST, SPI_MODE0, PIN_SPI_MOSI, PIN_SPI_SCK) {
}

void SPIWrapper::beginTransaction(const SPIWrapperSettings& settings) {
    use_bitbang = settings.clock < bitbang_threshold;
    current_settings = settings;

    // printf("beginning transaction(%ld, %d, %d, %d, %d) bb=%u\n", settings.clock, settings.bitOrder, settings.dataMode, settings.mosi_pin, settings.sck_pin, use_bitbang);

    if (use_bitbang) {
        pinMode(settings.mosi_pin, OUTPUT);
        pinMode(settings.sck_pin, OUTPUT);
        digitalWrite(settings.mosi_pin, LOW);
        digitalWrite(settings.sck_pin, (cpol() ? HIGH : LOW));
    } else {
        SPI.begin();
        SPI.setMOSI(PIN_SPI_MOSI);
        SPI.setSCK(PIN_SPI_SCK);

        if (settings.mosi_pin != PIN_SPI_MOSI) {
            printf("ERROR mosi pin must match builtin one\n");
        }
        if (settings.sck_pin != PIN_SPI_SCK) {
            printf("ERROR mosi pin must match builtin one\n");
        }
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

        digitalWrite(current_settings.mosi_pin, bit);

        if (!cpha()) {
            toggleClock();
        } else {
            delayBit();
            toggleClock();
        }
    }

    // restore clock to idle
    digitalWrite(current_settings.sck_pin, cpol() ? HIGH : LOW);
}

void SPIWrapper::toggleClock() {
    bool idle = cpol();
    bool active = !idle;

    digitalWrite(current_settings.sck_pin, active);
    delayBit();
    digitalWrite(current_settings.sck_pin, idle);
    delayBit();
}

SPIWrapper spiWrapper;