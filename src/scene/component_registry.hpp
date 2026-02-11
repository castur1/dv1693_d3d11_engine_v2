#ifndef COMPONENT_REGISTRY_HPP
#define COMPONENT_REGISTRY_HPP

#include "core/uuid.hpp"

#include <DirectXMath.h>

#include <string>
#include <unordered_map>
#include <functional>

using namespace DirectX;

class Entity;
class Component;

class ComponentRegistry {
public:
    struct Inspector {
        virtual void Field(const std::string &name, int          &val) = 0;
        virtual void Field(const std::string &name, unsigned int &val) = 0;
        virtual void Field(const std::string &name, float        &val) = 0;
        virtual void Field(const std::string &name, bool         &val) = 0;
        virtual void Field(const std::string &name, std::string  &val) = 0;
        virtual void Field(const std::string &name, XMFLOAT2     &val) = 0;
        virtual void Field(const std::string &name, XMFLOAT3     &val) = 0;
        virtual void Field(const std::string &name, XMFLOAT4     &val) = 0;
        virtual void Field(const std::string &name, UUID_        &val) = 0;
        // TODO: ...
    };

private:
    struct Component_funtions {
        std::function<Component *(Entity *, bool)> createFunc;
        std::function<void(Component *, Inspector *)> reflectFunc;
    };

public:
    static std::unordered_map<std::string, Component_funtions> &GetMap() {
        static std::unordered_map<std::string, Component_funtions> map;
        return map;
    }

    template <typename T>
    static bool Register(const std::string &name) {
        Component_funtions entry;

        entry.createFunc = [](Entity *owner, bool isActive) { return new T(owner, isActive); };

        entry.reflectFunc = [](Component *instance, Inspector *inspector) {
            static_cast<T *>(instance)->Reflect(inspector);
        };

        GetMap()[name] = entry;

        return true;
    }
};

#define REGISTER_COMPONENT(type) \
     inline const bool registered_##type = ComponentRegistry::Register<type>(#type)

#define BIND(variable) \
    inspector->Field(#variable, variable)

#endif