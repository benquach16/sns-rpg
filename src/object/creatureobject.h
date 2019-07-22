#pragma once

#include <map>

#include "object.h"
#include "../creatures/creature.h"
#include "../components/aicontroller.h"
#include "types.h"

class Level;

class CreatureObject : public Object
{
public:
	CreatureObject(Creature* creature);
	virtual ~CreatureObject();
	bool hasCollision() const override { return true; }
	bool deleteMe() const override { return m_creature->getCreatureState() == eCreatureState::Dead; }
	bool isConscious() const { return (!deleteMe() && m_creature->getCreatureState() != eCreatureState::Unconscious); }
	Creature* getCreatureComponent() const { return m_creature; }
	const std::string getName() const { return m_creature->getName(); }
	virtual eCreatureFaction getFaction() const { return m_creatureFaction; }
	virtual eCreatureRace getRace() const = 0;
	virtual eObjectTypes getObjectType() const { return eObjectTypes::Creature; }
	virtual bool isPlayer() const { return false; }
	bool isInCombat() const { return m_creature->getCreatureState() == eCreatureState::InCombat; }
	void run(const Level*) override;

	void addItem(int id, int count) { m_inventory[id] = count; }
	void addItem(int id) { m_inventory[id]++; }
	const std::map<int, int> &getInventory() { return m_inventory; }

protected:
	Creature* m_creature;
	eCreatureFaction m_creatureFaction;
	eCreatureRace m_creatureRace;

	AIController m_controller;

	std::map<int, int> m_inventory;
	int m_money;

	int m_thirst;
	int m_hunger;
	int m_exhaustion;
};
