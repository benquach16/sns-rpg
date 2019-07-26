#include <iostream>

#include "creature.h"
#include "../3rdparty/random.hpp"
#include "../dice.h"

using namespace std;

Creature::Creature() : m_BTN(cBaseBTN), m_brawn(1), m_agility(1),
					   m_cunning(1), m_perception(1), m_will(1), m_primaryWeaponId(0), m_combatPool(0),
					   m_currentState(eCreatureState::Idle), m_bonusDice(0), m_bloodLoss(0),
					   m_hasOffense(false), m_hasDefense(false), m_hasPosition(false)
{
	
}

const Weapon* Creature::getPrimaryWeapon() const
{
	return WeaponTable::getSingleton()->get(m_primaryWeaponId);
}

std::vector<const Armor*> Creature::getArmor() const
{
	std::vector<const Armor*> ret;
	for (int i = 0; i < m_armor.size(); ++i) {
		ret.push_back(ArmorTable::getSingleton()->get(m_armor[i]));
	}
	return ret;
}

void Creature::setWeapon(int idx)
{
	const Weapon* weapon = WeaponTable::getSingleton()->get(idx);
	m_primaryWeaponId = idx;
}

void Creature::inflictWound(Wound* wound, bool manueverFirst)
{
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

	if(wound->causesDeath() == true) {
		m_currentState = eCreatureState::Dead;
	}
	if(wound->immediateKO() == true) {
		m_currentState = eCreatureState::Unconscious;
	}
	set<eEffects> effects = wound->getEffects();
	auto BL1 = effects.find(eEffects::BL1);
	auto BL2 = effects.find(eEffects::BL2);
	auto BL3 = effects.find(eEffects::BL3);
	if(BL1 != effects.end()) {
		m_bloodLoss++;
	}
	if(BL2 != effects.end()) {
		m_bloodLoss+=2;
	}
	if(BL3 != effects.end()) {
		m_bloodLoss+=3;
	}
	auto KO1 = effects.find(eEffects::KO1);
	auto KO2 = effects.find(eEffects::KO2);
	auto KO3 = effects.find(eEffects::KO3);
	auto KO = effects.find(eEffects::KO);
	if(KO1 != effects.end()) {
		if(DiceRoller::rollGetSuccess(m_BTN, getGrit()) < 1) {
			m_currentState = eCreatureState::Unconscious;	
		}
	}
	if(KO2 != effects.end()) {
		if(DiceRoller::rollGetSuccess(m_BTN, getGrit()) < 2) {
			m_currentState = eCreatureState::Unconscious;	
		}
	}
	if(KO3 != effects.end()) {
		if(DiceRoller::rollGetSuccess(m_BTN, getGrit()) < 3) {
			m_currentState = eCreatureState::Unconscious;	
		}
	}
	if(KO != effects.end()) {
		m_currentState = eCreatureState::Unconscious;	
	}
	auto KD1 = effects.find(eEffects::KD1);
	auto KD2 = effects.find(eEffects::KD2);
	auto KD3 = effects.find(eEffects::KD3);
	auto KD = effects.find(eEffects::KD);
	if(KD1 != effects.end()) {
		if(DiceRoller::rollGetSuccess(m_BTN, getReflex()) < 1) {
			m_currentStance = eCreatureStance::Prone;
		}
	}
	if(KD2 != effects.end()) {
		if(DiceRoller::rollGetSuccess(m_BTN, getReflex()) < 2) {
			m_currentStance = eCreatureStance::Prone;
		}
	}
	if(KD3 != effects.end()) {
		if(DiceRoller::rollGetSuccess(m_BTN, getReflex()) < 3) {
			m_currentStance = eCreatureStance::Prone;
		}
	}
	if(KD != effects.end()) {
		m_currentStance = eCreatureStance::Prone;
	}	
	if (m_bloodLoss >= cBaseBloodLoss)
	{
		m_currentState = eCreatureState::Unconscious;
	}
	m_BTN = max(m_BTN, wound->getBTN());
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

void Creature::equipArmor(int id)
{
	const Armor *armor = ArmorTable::getSingleton()->get(id);
	assert(armor != nullptr);

	//make sure it doesnt overlap with another armor
	for(int i : m_armor) {
		const Armor* equippedArmor = ArmorTable::getSingleton()->get(i);
		assert(armor->isOverlapping(equippedArmor) == false);
	}

	m_armor.push_back(id);
	applyArmor();
}

bool Creature::canEquipArmor(int id)
{
	const Armor *armor = ArmorTable::getSingleton()->get(id);
	assert(armor != nullptr);
	for(int i : m_armor) {
		const Armor* equippedArmor = ArmorTable::getSingleton()->get(i);
		if(armor->isOverlapping(equippedArmor) == true) {
			return false;
		}
	}
	return true;
}

void Creature::removeArmor(int id)
{
	m_armor.erase(remove(m_armor.begin(), m_armor.end(), id), m_armor.end());
	applyArmor();
}

void Creature::resetCombatPool()
{
	//carryover impact damage across tempos
	const Weapon* weapon = getPrimaryWeapon();
	int carry = m_combatPool;
	carry = min(0, carry);
	m_combatPool = getProficiency(weapon->getType()) + getReflex() + carry;
	m_combatPool -= static_cast<int>(m_AP);
	cout << getName() << m_combatPool << endl;
}

void Creature::addAndResetBonusDice()
{
	m_currentOffense.dice += m_bonusDice;
	m_bonusDice = 0;
}

void Creature::doOffense(const Creature* target, int reachCost, bool allin, bool dualRedThrow)
{
	const Weapon* weapon = getPrimaryWeapon();

	m_currentOffense.manuever = eOffensiveManuevers::Thrust;
	m_currentOffense.component = weapon->getBestAttack();
	if(m_currentOffense.component->getAttack() == eAttacks::Swing) {
		m_currentOffense.manuever = eOffensiveManuevers::Swing;
	}

	//replace me
	m_currentOffense.target =
		target->getHitLocations()[effolkronium::random_static::get(0, (int)target->getHitLocations().size() - 1)];
	int dice = m_combatPool / 2 + effolkronium::random_static::get(0, m_combatPool/3)
		- effolkronium::random_static::get(0, m_combatPool/3);

	//bound
	dice += reachCost;
	dice = max(0, dice);
	dice = min(dice, m_combatPool);
	//never issue 0 dice for attack
	if(m_combatPool > 0 && dice == 0) {
		dice = 1;
	}
	if(allin == true) {
		m_currentOffense.dice = m_combatPool;
	} else {
		m_currentOffense.dice = dice;
	}

	if(dualRedThrow == true && m_combatPool > 0 && rand() % 2 == 1) {
		m_currentDefense.manuever = eDefensiveManuevers::StealInitiative;
		m_currentDefense.dice = m_combatPool - m_currentOffense.dice;

		//hacky since usually this happens in combatinstance
		m_combatPool -= m_currentDefense.dice;
	}
}


void Creature::doDefense(const Creature* attacker, bool isLastTempo)
{
	int diceAllocated = attacker->getQueuedOffense().dice;

	if(diceAllocated + 3 < m_combatPool) {
		m_currentDefense.manuever = eDefensiveManuevers::ParryLinked;
		m_combatPool -= 1;
	}
	else
	{
		m_currentDefense.manuever = eDefensiveManuevers::Parry;
	}

	int stealDie = 0;
	if(stealInitiative(attacker, stealDie) == true) {
		m_currentDefense.manuever = eDefensiveManuevers::StealInitiative;
		m_currentDefense.dice = stealDie;
		return;
	}
	if(isLastTempo == true) {
		//use all dice because we're going to refresh anyway
		m_currentDefense.dice = m_combatPool;
		m_currentDefense.dice = max(m_currentDefense.dice, 0);
		return;
	}
	int dice = std::min(diceAllocated + effolkronium::random_static::get(0, diceAllocated/3)
					   - effolkronium::random_static::get(0, diceAllocated/3)
					   , m_combatPool);
	dice = min(m_combatPool, dice);
	dice = max(dice, 0);
	m_currentDefense.dice = dice;
}

bool Creature::stealInitiative(const Creature* attacker, int& outDie)
{
	int diceAllocated = attacker->getQueuedOffense().dice;

	int combatPool = attacker->getCombatPool() + attacker->getSpeed();

	int bufferDie = effolkronium::random_static::get(1, 5);
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

void Creature::doStolenInitiative(const Creature* defender, bool allin)
{
	m_currentDefense.manuever = eDefensiveManuevers::StealInitiative;
	Defense defend = defender->getQueuedDefense();
	m_currentDefense.dice = min(m_combatPool, defend.dice);
	if(allin == true) {
		m_currentDefense.dice = m_combatPool;
	}
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

void Creature::clearCreatureManuevers()
{

}

void Creature::clearArmor()
{
	m_AP = 0;
	for(auto it : m_armorValues) {
		it.second.AV = 0;
		it.second.isMetal = false;
		it.second.isRigid = false;
		it.second.type = eArmorTypes::None;
	}
}

void Creature::applyArmor()
{
	clearArmor();
	for(int i : m_armor) {
		const Armor* armor = ArmorTable::getSingleton()->get(i);
		for(auto it : armor->getCoverage()) {
			m_AP += armor->getAP();
			m_armorValues[it].AV = max(m_armorValues[it].AV, armor->getAV());
			m_armorValues[it].isMetal = m_armorValues[it].isMetal || armor->isMetal();
			m_armorValues[it].isRigid = m_armorValues[it].isRigid || armor->isRigid();
			m_armorValues[it].type = max(armor->getType(), m_armorValues[it].type);
		}
	}

}
							 
