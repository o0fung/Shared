#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <Preferences.h>

class Memory {
    private:
        Preferences _pref;

    public:
        Memory();

        void init(char *name);
        
        void save_uint32(const char* key, const uint32_t val);
        uint32_t load_uint32(const char* key, const uint32_t default_val);
};

#endif
