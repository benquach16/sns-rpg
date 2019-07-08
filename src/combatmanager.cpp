#include <iostream>
#include "creatures/utils.h"
#include "items/utils.h"
#include "combatmanager.h"
#include "game.h"
#include "creatures/player.h"

#include <assert.h>

using namespace std;

CombatManager::CombatManager(): m_initiative(eInitiative::Side1), m_side1(nullptr), m_side2(nullptr),
								m_currentTempo(eTempo::First), m_currentState(eCombatState::Uninitialized)
{
}

void CombatManager::setSides(Creature*& attacker, Creature*& defender)
{
	if(m_initiative == eInitiative::Side1) {
		attacker = m_side1;
		defender = m_side2;
	}
	else if(m_initiative == eInitiative::Side2) {
		attacker = m_side2;
		defender = m_side1;
	}
	assert(attacker != nullptr);
	assert(defender != nullptr);
}

void CombatManager::initCombat(Creature* side1, Creature* side2)
{
	assert(side1 != nullptr);
	assert(side2 != nullptr);
	assert(m_currentState == eCombatState::Uninitialized);
	
	m_side1 = side1;
	m_side1->resetCombatPool();
	m_side2 = side2;
	m_side2->resetCombatPool();

	m_currentTempo = eTempo::First;
	m_initiative = eInitiative::Side1;
	m_currentState = eCombatState::Initialized;
}

void CombatManager::doInitialization()
{
	assert(m_side1 != nullptr);
	assert(m_side2 != nullptr);
	assert(m_currentState == eCombatState::Initialized);
	
	writeMessage("Combat between " + m_side1->getName() + " and " + m_side2->getName(), Log::eMessageTypes::Announcement);
	writeMessage(m_side1->getName() + " is using " + m_side1->getPrimaryWeapon()->getName() + " and " +
			  m_side2->getName() + " is using " + m_side2->getPrimaryWeapon()->getName());

	m_currentState = eCombatState::RollInitiative;
}

void CombatManager::doRollInitiative()
{
	if(m_side1->isPlayer() == true) {
		Player* player = static_cast<Player*>(m_side1);
		if(player->pollForInitiative() == false) {
			m_currentState = eCombatState::RollInitiative;
			return;			
		}
		
	}
	
	//get initiative rolls from both sides to determine roles.
	eInitiativeRoll side1 = m_side1->doInitiative();
	eInitiativeRoll side2 = m_side2->doInitiative();
	if(m_side1->isPlayer() == true) {
		Player* player = static_cast<Player*>(m_side1);
		side1 = player->getInitiative();
	}
	
	if(side1 == eInitiativeRoll::Defend && side2 == eInitiativeRoll::Defend) {
		//repeat
		writeMessage("Both sides chose to defend, deciding initiative again");
		m_currentState = eCombatState::ResetState;
		return;
	} else if(side1 == eInitiativeRoll::Attack && side2 == eInitiativeRoll::Defend) {
		writeMessage(m_side1->getName() + " chose to attack and " + m_side2->getName() + " is defending");
		m_initiative = eInitiative::Side1;
		m_currentState = eCombatState::Offense;
		return;
	} else if(side1 == eInitiativeRoll::Defend && side2 == eInitiativeRoll::Attack) {
		writeMessage(m_side2->getName() + " chose to attack and " + m_side1->getName() + " is defending");
		m_initiative = eInitiative::Side2;
		m_currentState = eCombatState::Offense;		
		return;
	} else if(side1 == eInitiativeRoll::Attack && side2 == eInitiativeRoll::Attack) {
		writeMessage("Both sides chose to attack, no defense can be done by either side.");
		// no defense here!
		// hopefully you don't die horribly
		// roll speed to determine who goes first
		int side1Successes = DiceRoller::rollGetSuccess(m_side1->getBTN(), m_side1->getSpeed());
		int side2Successes = DiceRoller::rollGetSuccess(m_side2->getBTN(), m_side2->getSpeed());

		m_initiative = side1Successes < side2Successes ? eInitiative::Side1 : eInitiative::Side2;
		if(m_initiative == eInitiative::Side1) {
			writeMessage(m_side1->getName() + " declares their attack first");
		} else {
			writeMessage(m_side2->getName() + " declares their attack first");
		}
		m_currentState = eCombatState::DualOffense1;
		return;
	}

	m_currentState = eCombatState::RollInitiative;
}

void CombatManager::doResetState()
{
	m_currentState = eCombatState::RollInitiative;
}

bool CombatManager::doOffense()
{
	//get offensive manuever and dice from side 1
	//then get defensive manuever and dice from side 2
	//then resolve		
	Creature* attacker = nullptr;
	Creature* defender = nullptr;
	setSides(attacker, defender);
	
	if(attacker->getCombatPool() <= 0 && defender->getCombatPool() > 0) {
		writeMessage(attacker->getName() + " has no more action points! Initiative swaps to defender");
		switchInitiative();
		setSides(attacker, defender);
	}
	
	Weapon* offenseWeapon = attacker->getPrimaryWeapon();
	Weapon* defenseWeapon = defender->getPrimaryWeapon();
	
	int offenseCombatPool = attacker->getCombatPool();

	int reachCost = static_cast<int>(defenseWeapon->getLength()) - static_cast<int>(offenseWeapon->getLength());
	if(attacker->isPlayer() == true)
	{
		//wait until we get input from player
		Player* player = static_cast<Player*>(attacker);
		if(player->pollForOffense() == false) {
			m_currentState = eCombatState::Offense;
			return false;
		} 
	}
	else {
		attacker->doOffense(defender, reachCost);		
	}
	reachCost = std::max(0, reachCost);
	Creature::Offense attack = attacker->getQueuedOffense();
	
	assert(attack.component != nullptr);
	assert(attack.dice <= offenseCombatPool);
	attacker->reduceCombatPool(attack.dice);
	
	writeMessage(attacker->getName() + " " + offensiveManueverToString(attack.manuever) + "s with " +
				 offenseWeapon->getName() + " using " +
				 attack.component->getName() + " with " +
				 to_string(attack.dice) + " action points");
	
	m_currentState = eCombatState::Defense;
	return true;
}

void CombatManager::doDualOffense1()
{
	//both sides rolled red
	Creature* attacker = nullptr;
	Creature* defender = nullptr;
	setSides(attacker, defender);
	//person who rolled better on speed goes second

	if(doOffense() == false) {
		m_currentState = eCombatState::DualOffense1;
		return;
	}
	switchInitiative();
	m_currentState = eCombatState::DualOffense2;
}

void CombatManager::doDualOffense2()
{
	//both sides rolled red
	Creature* attacker = nullptr;
	Creature* defender = nullptr;
	setSides(attacker, defender);

	if(doOffense() == false) {
		m_currentState = eCombatState::DualOffense2;
		return;
	}
	m_currentState = eCombatState::DualOffenseResolve;
}

void CombatManager::doDefense()
{
	Creature* attacker = nullptr;
	Creature* defender = nullptr;
	setSides(attacker, defender);

	Weapon* defenseWeapon = defender->getPrimaryWeapon();

	int defenseCombatPool = defender->getProficiency(defenseWeapon->getType()) + defender->getReflex();
	if(defender->isPlayer() == true) {
		//wait until player inputs
		Player* player = static_cast<Player*>(defender);
		if(player->pollForDefense() == false) {
			m_currentState = eCombatState::Defense;
			return;
		}

	}
	else {
		defender->doDefense(attacker, m_currentTempo == eTempo::Second);		
	}
	
	Creature::Defense defend = defender->getQueuedDefense();
	if(defend.manuever == eDefensiveManuevers::StealInitiative) {
		//need both sides to attempt to allocate dice
		m_currentState = eCombatState::StealInitiative;
	}
	assert(defend.dice <= defenseCombatPool);

	defender->reduceCombatPool(defend.dice);
	
	writeMessage(defender->getName() + " defends with " + defenseWeapon->getName() +
				 " using " + to_string(defend.dice) + " action points");
	
	m_currentState = eCombatState::Resolution;
}

void CombatManager::doStealInitiative()
{
	//defender inputs offense and dice to steal initiative
	Creature* attacker = nullptr;
	Creature* defender = nullptr;
	setSides(attacker, defender);

	//then input manuever
	Creature::Defense defend = defender->getQueuedDefense();
	Weapon* offenseWeapon = attacker->getPrimaryWeapon();
	Weapon* defenseWeapon = defender->getPrimaryWeapon();
	//do dice to steal initiative first
	if(defender->isPlayer() == true) {
		m_currentState = eCombatState::StealInitiative;
		return;
	}
	else {
		defender->reduceCombatPool(defend.dice);
		defender->doOffense(attacker, reachCost);
	}

	int reachCost = static_cast<int>(defenseWeapon->getLength()) - static_cast<int>(offenseWeapon->getLength());
	writeMessage(defender->getName() + " attempts to steal intiative using " + to_string(defend.dice) +
		" action points!");


	m_currentState = eCombatState::StolenOffense;
}

void CombatManager::doStolenOffense()
{
	Creature* attacker = nullptr;
	Creature* defender = nullptr;
	setSides(attacker, defender);

	if(attacker->isPlayer() == true) {
		m_currentState = eCombatState::StolenOffense;
		return;
	}

	
	
	m_currentState = eCombatState::doResolution;
}

void CombatManager::doResolution()
{
	Creature* attacker = nullptr;
	Creature* defender = nullptr;
	setSides(attacker, defender);

	Creature::Offense attack = attacker->getQueuedOffense();
	Creature::Defense defend = defender->getQueuedDefense();
	
	//roll dice
	int offenseSuccesses = DiceRoller::rollGetSuccess(attacker->getBTN(), attack.dice);
	int defenseSuccesses = DiceRoller::rollGetSuccess(defender->getBTN(), defend.dice);

	int MoS = offenseSuccesses - defenseSuccesses;

	if(MoS > 0) {
		if(inflictWound(MoS, attack, defender) == true) {
			m_currentState = eCombatState::FinishedCombat;
			return;
		}
	}
	else if (MoS == 0) {
		//nothing happens
		writeMessage("no net successes");
	}
	else if (defend.manuever != eDefensiveManuevers::Dodge) {
		writeMessage("attack deflected with " + to_string(-MoS) + " successes");
		writeMessage(defender->getName() + " now has initative, becoming attacker");
		switchInitiative();
	}	
	switchTempo();
	m_currentState = eCombatState::Offense;
}

void CombatManager::doDualOffenseResolve()
{
	//dual aggression
	Creature::Offense attack = m_side1->getQueuedOffense();
	Creature::Offense attack2 = m_side2->getQueuedOffense();

	int MoS = DiceRoller::rollGetSuccess(m_side1->getBTN(), attack.dice);
	int MoS2 = DiceRoller::rollGetSuccess(m_side2->getBTN(), attack2.dice);

	//resolve both
	bool death = false;
	if(MoS > 0) {
		if(inflictWound(MoS, attack, m_side2) == true) {
			death = true;
		}
	}
	if(MoS2 > 0) {
		if(inflictWound(MoS2, attack2, m_side1) == true) {
			death = true;
		}
	}

	switchTempo();
	
	//intiative goes to whoever got more hits
	m_currentState = eCombatState::Offense;
	if(MoS > MoS2) {
		m_initiative = eInitiative::Side1;
	} else if (MoS < MoS2) {
		m_initiative = eInitiative::Side2;
	} else {
		//reroll if no one died
		m_currentState = eCombatState::RollInitiative;
	}
	m_currentState = death == true ? eCombatState::FinishedCombat : m_currentState;
}

void CombatManager::doEndCombat()
{
	writeMessage("Combat has ended", Log::eMessageTypes::Announcement);
	m_side1 = nullptr;
	m_side2 = nullptr;
	m_currentState = eCombatState::Uninitialized;
}

bool CombatManager::inflictWound(int MoS, Creature::Offense attack, Creature* target)
{
	eBodyParts bodyPart = WoundTable::getSingleton()->getSwing(attack.target);

	int finalDamage = MoS + attack.component->getDamage();

	writeMessage("inflicted level " + to_string(finalDamage) + " wound to " + bodyPartToString(bodyPart));
	Wound* wound = WoundTable::getSingleton()->getWound(attack.component->getType(), bodyPart, finalDamage);
	writeMessage(wound->getText(), Log::eMessageTypes::Damage);
	if(wound->getBTN() > target->getBTN())
	{
		writeMessage(target->getName() + " begins to struggle from the pain", Log::eMessageTypes::Alert);
	}
	target->inflictWound(wound);

	if(wound->causesDeath() == true) {
		//end combat
		writeMessage(target->getName() + " has been killed", Log::eMessageTypes::Announcement);
		m_currentState = eCombatState::FinishedCombat;
		return true;
	}

	writeMessage("Wound impact causes " + target->getName() + " to lose " +
				 to_string(wound->getImpact()) + " action points!", Log::eMessageTypes::Alert);
	return false;
}

void CombatManager::switchTempo()
{
	if(m_currentTempo == eTempo::First) {
		m_currentTempo = eTempo::Second;
	} else {
		// reset combat pools
		writeMessage("Exchange has ended, combat pools have reset");
		m_currentTempo = eTempo::First;
		m_side1->resetCombatPool();
		m_side2->resetCombatPool();
	}
}

bool CombatManager::isAttackerPlayer()
{
	Creature* attacker = nullptr;
	Creature* defender = nullptr;
	setSides(attacker, defender);
	return attacker->isPlayer();
}

bool CombatManager::isDefenderPlayer()
{
	Creature* attacker = nullptr;
	Creature* defender = nullptr;
	setSides(attacker, defender);
	return defender->isPlayer();
}

void CombatManager::run()
{
	switch(m_currentState)
	{
	case eCombatState::Uninitialized:
		return;
		break;
	case eCombatState::Initialized:
		doInitialization();
		break;
	case eCombatState::RollInitiative:
		doRollInitiative();
		break;
	case eCombatState::ResetState:
		doResetState();
		break;
	case eCombatState::Offense:
		doOffense();
		break;
	case eCombatState::StealInitiative:
		doStealInitiative();
		break;
	case eCombatState::DualOffense1:
		doDualOffense1();
		break;
	case eCombatState::DualOffense2:
		doDualOffense2();
		break;
	case eCombatState::Defense:
		doDefense();
		break;
	case eCombatState::Resolution:
		doResolution();
		break;
	case eCombatState::DualOffenseResolve:
		doDualOffenseResolve();
		break;
	case eCombatState::FinishedCombat:
		doEndCombat();
		break;
	}
}

void CombatManager::runUI()
{
	Game::getWindow();
}

void CombatManager::writeMessage(const std::string& str, Log::eMessageTypes type)
{
	//combat manager is not a singleton, so we can have multiple.
	//we can choose not to display combatmanager messages if we want to.
	Log::push(str, type);
}
