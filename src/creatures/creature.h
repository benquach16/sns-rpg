#pragma once

#include "wound.h"
#include "types.h"
#include "../items/types.h"
#include "../items/weapon.h"

#include <vector>
#include <map>

static constexpr int cBaseBTN = 3;

class Creature
{
public:
	Creature();
	
	int getBrawn() { return m_brawn; }
	int getAgility() { return m_agility; }
	int getCunning() { return m_cunning; }
	int getPerception() { return m_perception; }
	int getWill() { return m_will; }
	
	int getGrit() { return (m_brawn + m_will)/2; }
	int getKeen() { return (m_cunning + m_perception)/2; }
	int getReflex() { return (m_agility + m_cunning)/2; }
	int getSpeed() { return (m_agility + m_brawn)/2; }

	int getBTN() { return m_BTN; }
	void setBTN(int BTN) { m_BTN = BTN; }

	Weapon* getPrimaryWeapon();
	void setWeapon(int id) { m_primaryWeaponId = id; }

	void setName(const std::string& name) { m_name = name; }
	std::string getName() { return m_name; }

	int getProficiency(eWeaponTypes type) { return m_proficiencies[type]; }

	void inflictWound(Wound* wound);

	int getSuccessRate();

	// for current weapon
	int getCombatPool() { return m_combatPool; }
	void resetCombatPool();
	void reduceCombatPool(int num) { m_combatPool -= num; }

	// AI functions
	virtual bool isPlayer() { return false; }
	void doOffense(Creature* target, int reachCost);

	void doDefense(Creature* attacker, bool isLastTempo);

	void doStolenInitiative();

	bool stealInitiative(Creature* attacker, int& outDie);

	eInitiativeRoll doInitiative();

	struct Offense {
		eOffensiveManuevers manuever;
		int dice;
		eHitLocations target;
		Component* component = nullptr;
	};
	//this is reused in stealing initiative to hold die allocated to stealing
	struct Defense {
		eDefensiveManuevers manuever;
		int dice;
	};

	Offense getQueuedOffense() { return m_currentOffense; }
	Defense getQueuedDefense() { return m_currentDefense; }
	
protected:
	std::vector<eHitLocations> m_hitLocations;
	std::map<eBodyParts, int> m_armor;
	std::vector<Wound*> m_wounds;

	std::string m_name;

	Offense m_currentOffense;
	Defense m_currentDefense;

	//index
	int m_primaryWeaponId;
	
	int m_bloodLoss;
	int m_BTN;

	bool m_isPlayer;

	//stats
	int m_brawn;
	int m_agility;
	int m_cunning;
	int m_perception;
	int m_will;

	int m_combatPool;

	std::map<eWeaponTypes, int> m_proficiencies;
	
};
