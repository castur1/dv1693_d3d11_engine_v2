#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <unordered_map>
#include <string>

class Debug {
    static std::unordered_map<std::string, bool> settings;
    static std::unordered_map<std::string, std::string> stats;

public:
    static void NewFrame();

    static void SetSetting(const std::string &name, bool value);
    static bool GetSetting(const std::string &name, bool defaultValue = false);

    static void SetStat(const std::string &name, const std::string &value);
    static void SetStat(const std::string &name, int value);
    static void SetStat(const std::string &name, float value);
    static void SetStat(const std::string &name, bool value);

    static const std::unordered_map<std::string, bool> &GetCurrentSettings();
    static const std::unordered_map<std::string, std::string> &GetCurrentStats();
};

#endif