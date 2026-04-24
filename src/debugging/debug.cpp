#include "debug.hpp"

std::map<std::string, bool> Debug::settings;
std::map<std::string, std::string> Debug::stats;

void Debug::NewFrame() { 
    stats.clear(); 
}

void Debug::SetSetting(const std::string &name, bool value) { 
    settings[name] = value; 
}

bool Debug::GetSetting(const std::string &name, bool defaultValue) {
    auto iter = settings.find(name);
    if (iter != settings.end())
        return iter->second;

    return settings[name] = defaultValue;
}

void Debug::SetStat(const std::string &name, const std::string &value) { 
    stats[name] = value; 
}

void Debug::SetStat(const std::string &name, int value) { 
    stats[name] = std::to_string(value); 
}

void Debug::SetStat(const std::string &name, float value) { 
    stats[name] = std::to_string(value); 
}

void Debug::SetStat(const std::string &name, bool value) { 
    stats[name] = std::to_string(value); 
}

void Debug::SetStat(const std::string &name, XMFLOAT3 value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[%.2f, %.2f, %.2f]", value.x, value.y, value.z);
    stats[name]= buffer;
}

const std::map<std::string, bool> &Debug::GetCurrentSettings() {
    return settings;
}

const std::map<std::string, std::string> &Debug::GetCurrentStats() {
    return stats;
}