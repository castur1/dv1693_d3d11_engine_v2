#include "uuid.hpp"

#include <random>

static std::random_device randomDevice;
static std::mt19937_64 engine(randomDevice());
static std::uniform_int_distribution<uint64_t> uniformDistribution;

const UUID_ UUID_::invalid = 0;

UUID_::UUID_() {
    this->uuid = uniformDistribution(engine);
}

UUID_::UUID_(uint64_t uuid) : uuid(uuid) {}

bool UUID_::IsValid() const {
    return this->uuid != 0;
}

std::string UUID_::ToString() const {
    char buffer[20];
    snprintf(
        buffer,
        20,
        "%04llx-%04llx-%04llx-%04llx",
        (this->uuid >> 48) & 0xFFFF,
        (this->uuid >> 32) & 0xFFFF,
        (this->uuid >> 16) & 0xFFFF,
        this->uuid & 0xFFFF
    );

    return std::string(buffer);
}

UUID_ UUID_::FromString(const std::string &str) {
    uint64_t parts[4];
    if (sscanf_s(str.c_str(), "%04llx-%04llx-%04llx-%04llx", &parts[0], &parts[1], &parts[2], &parts[3]) != 4)
        return UUID_::invalid;

    return UUID_((parts[0] << 48) | (parts[1] << 32) | (parts[2] << 16) | parts[3]);
}