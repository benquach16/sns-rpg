#include <algorithm>
#include <iostream>

#include "playerobject.h"

using namespace std;

BOOST_CLASS_EXPORT(PlayerObject)

PlayerObject::PlayerObject()
    : CreatureObject(new Player)
    , m_toMove(0.f, 0.f)
{
    /*
    m_creature->setWeapon(1040);
    // m_creature->equipArmor(2055);
    // m_creature->equipArmor(2052);
    m_creature->equipArmor(2044);
    m_creature->equipArmor(2040);
    m_creature->equipArmor(2046);
    // m_creature->equipArmor(2043);
    // m_creature->equipArmor(2056);
    // m_creature->equipArmor(2057);
    m_creature->setName("John");

    m_inventory[0] = 50;
    m_inventory[1] = 5;
    m_inventory[2] = 1;
    m_inventory[3] = 1;
    m_inventory[4] = 1;
    m_inventory[5] = 3;
    m_inventory[6] = 1;
    */
}

PlayerObject::~PlayerObject() {}

void PlayerObject::run(const Level* level)
{
    /*
    if (m_manager->isParent() == true) {
        m_manager->run(0.9);
    }
    */

    m_toMove.x = 0;
    m_toMove.y = 0;
}

bool PlayerObject::runCombat(float tick)
{
    // we are in a duel but not parent
    if (isInCombat() == true && m_manager->isParent() == false) {
        return true;
    }
    if (isInCombat() == false) {
        m_creature->clearCreatureManuevers();
    }
    bool ret = m_manager->run(tick);
    if (ret == false) {
        setOutofCombat();
    }
    return ret;
}

void PlayerObject::moveDown() { m_position.y++; }

void PlayerObject::moveUp() { m_position.y--; }

void PlayerObject::moveLeft() { m_position.x--; }
void PlayerObject::moveRight() { m_position.x++; }
