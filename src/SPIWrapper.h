#pragma once

#include <Arduino.h>
#include <SPI.h>

class SPIWrapperSettings {
   public:
    uint32_t clock;
    uint8_t bitOrder;
    uint8_t dataMode;

    SPIWrapperSettings(uint32_t clk, uint8_t order, uint8_t mode);
};

class SPIWrapper {
   public:
    SPIWrapper(uint32_t bitbangThreshold = 1000000, uint8_t mosiPin = 11, uint8_t sckPin = 13);

    void beginTransaction(const SPIWrapperSettings& settings);
    void endTransaction();
    void transfer16(uint16_t data);

   private:
    uint32_t bitbang_threshold;
    uint8_t mosi_pin;
    uint8_t sck_pin;
    bool use_bitbang;
    SPIWrapperSettings current_settings;

    bool cpol() const;
    bool cpha() const;
    void delayBit() const;
    void bitbangTransfer16(uint16_t data);
    void toggleClock();
};

extern SPIWrapper spiWrapper;
