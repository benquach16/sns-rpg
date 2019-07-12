#pragma once

#include "numberinput.h"
#include "../creatures/player.h"

class OffenseUI
{
public:
	void run(sf::Event event, Player* player);
	void resetState() { m_currentState = eUiState::ChooseManuever; }
private:
	void doManuever(sf::Event event, Player* player);
	void doComponent(sf::Event event, Player* player);
	void doDice(sf::Event event, Player* player);
	void doTarget(sf::Event event, Player* player);
	
	enum class eUiState : unsigned {
		ChooseManuever,
		ChooseComponent,
		ChooseDice,
		ChooseTarget,
		Finished,
	};

	NumberInput m_numberInput;
	eUiState m_currentState;
};