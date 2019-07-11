#include <iostream>

#include "creature.h"
#include "../3rdparty/random.hpp"
#include "../dice.h"

using namespace std;

Creature::Creature() : m_BTN(cBaseBTN), m_brawn(1), m_agility(1),
					   m_cunning(1), m_perception(1), m_will(1), m_primaryWeaponId(0), m_combatPool(0)
{
	
}

Weapon* Creature::getPrimaryWeapon()
{
	return WeaponTable::getSingleton()->get(m_primaryWeaponId);
}

void Creature::inflictWound(Wound* wound, bool manueverFirst)
{
	m_BTN = max(m_BTN, wound->getBTN());
	if(manueverFirst == true) {
		if(wound->getImpact() > m_currentOffense.dice) {
			int diff = wound->getImpact() - m_currentOffense.dice;
			m_currentOffense.dice = 0;
			m_combatPool -= diff;
		}
		else {			
			m_currentOffense.dice -= wound->getImpact();
		}
	}
	else {
		m_combatPool -= wound->getImpact();
	}
	m_wounds.push_back(wound);
	m_bloodLoss;
}

int Creature::getSuccessRate() const {
	float sides = static_cast<float>(DiceRoller::cDiceSides);
	float btn = static_cast<float>(DiceRoller::cDiceSides - m_BTN) + 1.f;

	float val = btn / sides;
	val *= 100;
	return static_cast<int>(val);
}

ArmorSegment Creature::getArmorAtPart(eBodyParts part)
{
	return m_armorValues[part];
}

void Creature::resetCombatPool()
{
	//carryover impact damage across tempos
	Weapon* weapon = getPrimaryWeapon();
	int carry = m_combatPool;
	carry = min(0, carry);
	m_combatPool = getProficiency(weapon->getType()) + getReflex() + carry;
}

void Creature::doOffense(const Creature* target, int reachCost, bool allin)
{
	Weapon* weapon = getPrimaryWeapon();
	//we shouldn't be able to pgo below 0 with this
	m_combatPool -= reachCost;
	m_combatPool = max(0, m_combatPool);

	m_currentOffense.manuever = eOffensiveManuevers::Thrust;
	m_currentOffense.component = weapon->getBestAttack();
	if(m_currentOffense.component->getAttack() == eAttacks::Swing) {
		m_currentOffense.manuever = eOffensiveManuevers::Swing;
	}
	m_currentOffense.target = eHitLocations::Head;
	int dice = m_combatPool / 2 + effolkronium::random_static::get(0, m_combatPool/3)
		- effolkronium::random_static::get(0, m_combatPool/3);

	//bound
	dice = max(0, dice);
	dice = min(dice, m_combatPool);
	if(allin == true) {
		m_currentOffense.dice = m_combatPool;
	} else {
		m_currentOffense.dice = dice;
	}
}


void Creature::doDefense(const Creature* attacker, bool isLastTempo)
{
	int diceAllocated = attacker->getQueuedOffense().dice;

	m_currentDefense.manuever = eDefensiveManuevers::Parry;

	int stealDie = 0;
	if(stealInitiative(attacker, stealDie) == true) {
		m_currentDefense.manuever = eDefensiveManuevers::StealInitiative;
		m_currentDefense.dice = stealDie;
		cout << "using " << m_currentDefense.dice << endl;
		return;
	}
	if(isLastTempo == true) {
		//use all dice because we're going to refresh anyway
		m_currentDefense.dice = m_combatPool;
		return;
	}
	int dice = std::min(diceAllocated + effolkronium::random_static::get(0, m_combatPool/3)
					   - effolkronium::random_static::get(0, m_combatPool/3)
					   , m_combatPool);
	dice = min(m_combatPool, dice);
	dice = max(dice, 0);
	m_currentDefense.dice = dice;
}

bool Creature::stealInitiative(const Creature* attacker, int& outDie)
{
	int diceAllocated = attacker->getQueuedOffense().dice;

	int combatPool = attacker->getCombatPool() + attacker->getSpeed();

	constexpr int bufferDie = 2;
	if((combatPool * 1.5) + bufferDie < m_combatPool + getSpeed()) {
		int diff = (getSpeed() - attacker->getSpeed()) * 1.5;
		int dice = diff + bufferDie;
		if(m_combatPool > dice) {
			outDie = dice;
			return true;
		}

	}
	return false;
}

void Creature::doStolenInitiative(const Creature* defender)
{
	m_currentDefense.manuever = eDefensiveManuevers::StealInitiative;
	Defense defend = defender->getQueuedDefense();
	m_currentDefense.dice = min(m_combatPool, defend.dice);
}

eInitiativeRoll Creature::doInitiative()
{
	//do random for now
	//this should be based on other creatures weapon length and armor and stuff
	if(effolkronium::random_static::get(0, 1) == 1){
		return eInitiativeRoll::Attack;
	}
	return eInitiativeRoll::Defend;
}
							 
