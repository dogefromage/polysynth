#pragma once

#include <Arduino.h>
#include <SPI.h>

class SPIWrapperSettings {
   public:
    uint32_t clock;
    uint8_t bitOrder;
    uint8_t dataMode;
    uint8_t mosi_pin;
    uint8_t sck_pin;

    SPIWrapperSettings(uint32_t clk, uint8_t order, uint8_t mode, uint8_t mosiPin, uint8_t sckPin);
};

class SPIWrapper {
   public:
    SPIWrapper(uint32_t bitbangThreshold = 1000000);

    void beginTransaction(const SPIWrapperSettings& settings);
    void endTransaction();
    void transfer16(uint16_t data);

   private:
    uint32_t bitbang_threshold;
    bool use_bitbang;
    SPIWrapperSettings current_settings;

    bool cpol() const;
    bool cpha() const;
    void delayBit() const;
    void bitbangTransfer16(uint16_t data);
    void toggleClock();
};

extern SPIWrapper spiWrapper;
