#include <Arduino.h>
#include <Preferences.h>
#include "Memory.h"

Memory::Memory() {};

void Memory::init(char *name) {
    _pref.begin(name, false);
}

void Memory::save_uint32(const char* key, const uint32_t val) {
    _pref.putUInt(key, val);
}

uint32_t Memory::load_uint32(const char* key, const uint32_t default_val) {
    return _pref.getUInt(key, default_val);
}
