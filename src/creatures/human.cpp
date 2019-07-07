#include "human.h"

Human::Human() : Creature()
{
	m_hitLocations.push_back(eHitLocations::Head);
	m_hitLocations.push_back(eHitLocations::Arm);
	m_hitLocations.push_back(eHitLocations::Chest);
	m_hitLocations.push_back(eHitLocations::Belly);
	m_hitLocations.push_back(eHitLocations::Thigh);
	m_hitLocations.push_back(eHitLocations::Shin);

	m_proficiencies[eWeaponTypes::Polearms] = 6;
	m_proficiencies[eWeaponTypes::Swords] = 6;

	m_brawn = 4;
	m_agility = 4;
	m_cunning = 4;
	m_perception = 4;
	m_will = 4;
}



