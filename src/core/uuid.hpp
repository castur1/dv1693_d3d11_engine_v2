#ifndef UUID_HPP
#define UUID_HPP

#include <xhash>
#include <string>

class UUID_ {
    uint64_t uuid;

public:
    static const UUID_ invalid;

    UUID_();
    UUID_(uint64_t uuid);

    bool IsValid() const;

    std::string ToString() const;
    static UUID_ FromString(const std::string &str);
    static UUID_ FromHash(const std::string &str);

    operator uint64_t() const { return this->uuid; }

    bool operator==(const UUID_ &other) const { return this->uuid == other.uuid; }
    bool operator!=(const UUID_ &other) const { return this->uuid != other.uuid; }
};

namespace std {
    template<>
    struct hash<UUID_> {
        std::size_t operator()(const UUID_& uuid) const {
            return hash<uint64_t>()(uuid);
        }
    };
}

typedef UUID_ AssetID, EntityID;

#endif