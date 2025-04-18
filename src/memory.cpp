#include "memory.h"

void memory_load_buffer(uint8_t* dest, size_t eeprom_addr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        dest[i] = EEPROM.read(eeprom_addr + i);
    }
}

void memory_save_buffer(uint8_t* src, size_t eeprom_addr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        EEPROM.update(eeprom_addr + i, src[i]);
    }
}
