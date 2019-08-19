#include "utils.h"

std::string factionToString(eCreatureFaction faction)
{
    switch (faction) {
    case eCreatureFaction::Player:
        return "Player";
        break;
    case eCreatureFaction::Bandit:
        return "Bandit";
        break;
    case eCreatureFaction::Wildlife:
        return "Wildlife";
        break;
    case eCreatureFaction::EidgenConfederacy:
        return "Eidgen Confederacy";
        break;
    default:
        return "";
        break;
    }
}

std::string raceToString(eCreatureRace race)
{
    switch (race) {
    case eCreatureRace::Human:
        return "Human";
        break;
    case eCreatureRace::Goblin:
        return "Goblin";
        break;
    }
    return "";
}

eCreatureFaction stringToFaction(const std::string& str)
{
    if (str == "bandit") {
        return eCreatureFaction::Bandit;
    } else if (str == "wildlife") {
        return eCreatureFaction::Wildlife;
    } else if (str == "eidgen") {
        return eCreatureFaction::EidgenConfederacy;
    }
    return eCreatureFaction::None;
}

eCreatureRace stringToRace(const std::string& str)
{
    if (str == "goblin") {
        return eCreatureRace::Goblin;
    }
    return eCreatureRace::Human;
}
