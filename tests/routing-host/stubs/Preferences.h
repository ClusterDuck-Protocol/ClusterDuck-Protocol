#pragma once
#include <cstddef>
#include <map>
#include <string>
// Keeps integers in memory, so tests can see what a command stored.
struct Preferences {
    std::map<std::string, int> ints;
    bool begin(const char*) { return true; }
    bool isKey(const char* key) { return ints.count(key) != 0; }
    bool remove(const char* key) { return ints.erase(key) != 0; }
    int getInt(const char* key, int value) {
        auto found = ints.find(key);
        return found == ints.end() ? value : found->second;
    }
    float getFloat(const char*, float value) { return value; }
    size_t putInt(const char* key, int value) { ints[key] = value; return sizeof(int); }
    size_t putFloat(const char*, float) { return sizeof(float); }
};
