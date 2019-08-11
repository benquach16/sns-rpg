#pragma once

#include "../creatures/player.h"
#include "numberinput.h"

class PrecombatUI {
public:
    void run(sf::Event event, Player* player);
    void resetState() { m_currentState = eUiState::ChooseFavoring; }

private:
    void doFavoring(sf::Event event, Player* player);
    void doFavorLocation(sf::Event event, Player* player);
    void doChooseGrip(sf::Event event, Player* player);
    enum class eUiState : unsigned { ChooseFavoring, ChooseFavorLocations, ChooseGrip, Finished };

    eUiState m_currentState;
};