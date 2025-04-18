#pragma once

#include <EEPROM.h>

#define MEMORY_TUNING_START_ADDRESS 0
#define MEMORY_TUNING_SECTION_SIZE (8 * 2 * 3 * sizeof(float))

#define MEMORY_PRESETS_START_ADDRESS MEMORY_TUNING_SECTION_SIZE

void memory_load_buffer(uint8_t* dest, size_t eeprom_addr, size_t count);
void memory_save_buffer(uint8_t* src, size_t eeprom_addr, size_t count);