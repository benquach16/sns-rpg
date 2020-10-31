#pragma once
#include "../items/types.h"
#include "types.h"

enum class eManueverTypes { Offense, Defense, Position, PreResolve };

class Component;

struct Manuever {
    virtual eManueverTypes getType() = 0;
    int dice = 0;
};

struct Offense : public Manuever {
    eManueverTypes getType() override { return eManueverTypes::Offense; }
    eOffensiveManuevers manuever;
    bool linked = false;
    bool feint = false;
    bool pinpoint = false;
    bool stomp = false;
    int heavyblow = 0;
    eHitLocations target;
    eBodyParts pinpointTarget;
    Weapon* weapon;
    Component* component;
};

struct Defense : public Manuever {
    eManueverTypes getType() override { return eManueverTypes::Defense; }
    Weapon* weapon;
    eDefensiveManuevers manuever;
};

struct Position : public Manuever {
    eManueverTypes getType() override { return eManueverTypes::Position; }
    ePositionManuevers manuever;
};

struct PreResolve : public Manuever {
    eManueverTypes getType() override { return eManueverTypes::PreResolve; }
};
