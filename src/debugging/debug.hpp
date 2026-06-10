#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <map>
#include <string>

#include <DirectXMath.h>

using namespace DirectX;

class Debug {
    friend class Application;

    static std::map<std::string, bool> settings;
    static std::map<std::string, int> integerSettings;
    static std::map<std::string, std::string> stats;

    static void NewFrame();
public:

    static void SetSetting(const std::string &name, bool value);
    static bool GetSetting(const std::string &name, bool defaultValue = false);

    static void SetIntegerSetting(const std::string &name, int value);
    static int  GetIntegerSetting(const std::string &name, int defaultValue = 0);

    static void SetStat(const std::string &name, const std::string &value);
    static void SetStat(const std::string &name, int value);
    static void SetStat(const std::string &name, float value);
    static void SetStat(const std::string &name, bool value);
    static void SetStat(const std::string &name, XMFLOAT3 value);

    static const std::map<std::string, bool> &GetCurrentSettings();
    static const std::map<std::string, int> &GetCurrentIntegerSettings();
    static const std::map<std::string, std::string> &GetCurrentStats();
};

#endif