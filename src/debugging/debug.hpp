#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <map>
#include <string>

#include <DirectXMath.h>

using namespace DirectX;

class Debug {
    static std::map<std::string, bool> settings;
    static std::map<std::string, std::string> stats;

public:
    static void NewFrame();

    static void SetSetting(const std::string &name, bool value);
    static bool GetSetting(const std::string &name, bool defaultValue = false);

    static void SetStat(const std::string &name, const std::string &value);
    static void SetStat(const std::string &name, int value);
    static void SetStat(const std::string &name, float value);
    static void SetStat(const std::string &name, bool value);
    static void SetStat(const std::string &name, XMFLOAT3 value);

    static const std::map<std::string, bool> &GetCurrentSettings();
    static const std::map<std::string, std::string> &GetCurrentStats();
};

#endif