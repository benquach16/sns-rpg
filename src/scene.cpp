#include "scene.h"
#include "creatures/wound.h"
#include "game.h"
#include "gfxobjects/utils.h"
#include "items/weapon.h"
#include "level/level.h"
#include "log.h"
#include "object/humanobject.h"
#include "object/playerobject.h"
#include "object/relationmanager.h"
#include "object/selectorobject.h"

using namespace std;

constexpr float cMaxZoom = 0.6f;
constexpr float cMinZoom = 1.4f;
constexpr int cRestTicks = 20;

Scene::Scene()
    : m_currentState(eSceneState::Uninitialized)
    , m_pickup(nullptr)
    , m_talking(nullptr)
    , m_currentLevel(nullptr)
{
}

Scene::~Scene()
{
    if (m_currentLevel != nullptr) {
        delete m_currentLevel;
        m_currentLevel = nullptr;
    }
}

void Scene::setupLevel(PlayerObject* playerObject)
{
    m_currentLevel = new Level(60, 60);
    m_currentLevel->generate();
    for (unsigned i = 0; i < 60; i++) {
        (*m_currentLevel)(i, 0).m_type = eTileType::Wall;
    }

    for (unsigned i = 2; i < 60; i++) {
        (*m_currentLevel)(0, i).m_type = eTileType::Wall;
    }

    HumanObject* human1 = new HumanObject;
    human1->setFaction(eCreatureFaction::Confederacy);
    human1->setLoadout(eCreatureFaction::Confederacy, eRank::Soldier);
    human1->setPosition(2, 2);
    human1->setAIRole(eAIRoles::Standing);
    human1->setStartingDialogueLabel("greeting_intro");
    human1->getCreatureComponent()->setAgility(9);
    human1->getCreatureComponent()->setIntuition(9);
    human1->getCreatureComponent()->setPerception(9);
    human1->setName("Sir Wilhelm");
    m_talking = human1;
    m_ui.initDialog(human1);
    m_currentState = eSceneState::DialogueMode;
    m_currentLevel->addObject(human1);

    playerObject->setPosition(1, 1);
    // has some management of player here but cannot delete
    // violates RAII
    m_currentLevel->addObject(playerObject);
}

void Scene::run(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    sf::View v = Game::getWindow().getDefaultView();
    v.setSize(v.getSize().x, v.getSize().y * 2);
    // v.setCenter(v.getSize() *.5f);
    sf::Vector2f center(playerObject->getPosition().x, playerObject->getPosition().y);
    center = coordsToScreen(center);
    v.setCenter(center.x, center.y + 200);
    v.zoom(zoom);
    Game::getWindow().setView(v);
    m_gfxlevel.run(m_currentLevel, playerObject->getPosition());
    // temporary until we get graphics queue up and running
    if (m_currentState == eSceneState::AttackMode || m_currentState == eSceneState::SelectionMode
        || m_currentState == eSceneState::DialogueSelect) {
        m_gfxSelector.run(&m_selector);
    }
    m_gfxlevel.renderText();
    Game::getWindow().setView(Game::getWindow().getDefaultView());

    m_ui.run();

    if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Equal) {
        zoom = max(cMaxZoom, zoom - 0.1f);
    }
    if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Hyphen) {
        zoom = min(cMinZoom, zoom + 0.1f);
    }

    switch (m_currentState) {
    case eSceneState::Playing:
        playing(hasKeyEvents, event, playerObject);
        break;
    case eSceneState::SelectionMode:
        selection(hasKeyEvents, event, playerObject);
        break;
    case eSceneState::DialogueMode:
        dialog(hasKeyEvents, event, playerObject);
        break;
    case eSceneState::AttackMode:
        attack(hasKeyEvents, event, playerObject);
        break;
    case eSceneState::Inventory:
        inventory(hasKeyEvents, event, playerObject);
        break;
    case eSceneState::Pickup:
        pickup(hasKeyEvents, event, playerObject);
        break;
    case eSceneState::InCombat:
        combat(hasKeyEvents, event, playerObject);
        break;
    default:
        break;
    }
    if (playerObject->isConscious() == false && m_currentState != eSceneState::Dead) {
        m_currentState = eSceneState::Dead;
        Log::push("You have died!", Log::eMessageTypes::Damage);
    }

    Log::run();
    float currentTime = clock.restart().asSeconds();
    aiTick += currentTime;
    tick += currentTime;
}

void Scene::selection(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    if (hasKeyEvents && event.type == sf::Event::KeyReleased) {
        doMoveSelector(event, playerObject, false);
        if (event.key.code == sf::Keyboard::Enter) {
            const Object* object = m_currentLevel->getObject(m_selector.getPosition());
            if (object != nullptr) {
                Log::push("You see " + object->getDescription());
                if (object->getObjectType() == eObjectTypes::Creature) {
                    const CreatureObject* creatureObj = static_cast<const CreatureObject*>(object);
                    // don't do anything more for player
                    if (creatureObj->isPlayer() == false) {
                        if (creatureObj->isConscious() == false) {
                            Log::push("They are unconscious", Log::eMessageTypes::Announcement);
                        }
                        int relation = RelationManager::getSingleton()->getRelationship(
                            eCreatureFaction::Player, creatureObj->getFaction());

                        if (relation <= RelationManager::cHostile) {
                            Log::push(creatureObj->getName() + " is hostile to you",
                                Log::eMessageTypes::Damage);
                        }
                    }
                }
            }
        }

        if (event.key.code == sf::Keyboard::D) {
            m_currentState = eSceneState::Playing;
        }
    }
}

void Scene::playing(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    vector2d pos = playerObject->getPosition();
    if (hasKeyEvents && event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Down) {
            if (m_currentLevel->isFreeSpace(pos.x, pos.y + 1) == true) {
                playerObject->moveDown();
            }
        }
        if (event.key.code == sf::Keyboard::Up) {
            if (m_currentLevel->isFreeSpace(pos.x, pos.y - 1) == true) {
                playerObject->moveUp();
            }
        }
        if (event.key.code == sf::Keyboard::Left) {
            if (m_currentLevel->isFreeSpace(pos.x - 1, pos.y) == true) {
                playerObject->moveLeft();
            }
        }
        if (event.key.code == sf::Keyboard::Right) {
            if (m_currentLevel->isFreeSpace(pos.x + 1, pos.y) == true) {
                playerObject->moveRight();
            }
        }
    } else if (hasKeyEvents && event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::A) {
            m_selector.setPosition(playerObject->getPosition());
            m_currentState = eSceneState::AttackMode;
        }
        if (event.key.code == sf::Keyboard::I) {
            m_currentState = eSceneState::Inventory;
        }
        if (event.key.code == sf::Keyboard::D) {
            m_selector.setPosition(playerObject->getPosition());
            m_currentState = eSceneState::SelectionMode;
        }
        if (event.key.code == sf::Keyboard::P) {
            Object* object
                = m_currentLevel->getObjectMutable(playerObject->getPosition(), playerObject);
            if (object != nullptr) {
                switch (object->getObjectType()) {
                case eObjectTypes::Corpse:
                    Log::push("Searching corpse...");
                    m_pickup = object;
                    m_currentState = eSceneState::Pickup;
                    break;
                case eObjectTypes::Chest:
                    Log::push("Opening chest...");
                    m_pickup = object;
                    m_currentState = eSceneState::Pickup;
                    break;
                case eObjectTypes::Creature:
                    Log::push("There is a creature here. You need to kill "
                              "them if you want to loot them.",
                        Log::eMessageTypes::Alert);
                    break;
                default:
                    Log::push("Nothing to loot");
                    break;
                }
            } else {
                Log::push("There is nothing here.");
            }
        }
        if (event.key.code == sf::Keyboard::K) {
            m_selector.setPosition(playerObject->getPosition());
            m_currentState = eSceneState::DialogueSelect;
        }
        if (event.key.code == sf::Keyboard::R) {
            Log::push("Resting..");
            m_currentState = eSceneState::Waiting;
        }
    }

    if (playerObject->isInCombat() == true) {
        m_currentState = eSceneState::InCombat;
    }

    if (aiTick > cPlayingTick) {
        m_currentLevel->run();
        aiTick = 0;
        m_currentLevel->cleanup();
    }
}

void Scene::dialogSelect(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    if (hasKeyEvents && event.type == sf::Event::KeyReleased) {
        doMoveSelector(event, playerObject, true);
        if (event.key.code == sf::Keyboard::Enter) {
            Object* object
                = m_currentLevel->getObjectMutable(m_selector.getPosition(), playerObject);
            if (object != nullptr) {
                if (object->getObjectType() == eObjectTypes::Creature) {
                    CreatureObject* creatureObject = static_cast<CreatureObject*>(object);
                    if (creatureObject->isConscious() == true) {
                        m_talking = creatureObject;
                        m_ui.initDialog(m_talking);
                        m_currentState = eSceneState::DialogueMode;
                    } else {
                        Log::push("You can't talk to an unconscious creature");
                    }
                }
            }
        }
        if (event.key.code == sf::Keyboard::K) {
            m_currentState = eSceneState::Playing;
        }
    }
}

void Scene::inventory(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    m_ui.runInventory(hasKeyEvents, event, playerObject);
    if (hasKeyEvents && event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::I) {
        m_currentState = eSceneState::Playing;
    }
}

void Scene::combat(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    m_ui.runCombat(hasKeyEvents, event, playerObject->getCombatManager());

    if (playerObject->runCombat(tick) == false) {
        m_currentState = eSceneState::Playing;
    }
    /*
      if (playerObject->isInCombat() == false) {
      m_currentState = eSceneState::Playing;
      }
    */
    if (tick > CombatManager::cTick) {
        // issue here is if player is engaged and is not the parent. the ai updates so
        // slowly that its hard for the player to understand whats going on
        m_currentLevel->run();
        tick = 0;
    }
}

void Scene::dialog(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    assert(m_talking != nullptr);
    if (m_ui.runDialog(hasKeyEvents, event, playerObject, m_talking) == false) {
        m_currentState = eSceneState::Playing;
    }
}

void Scene::pickup(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    assert(m_pickup != nullptr);
    m_ui.runTrade(
        hasKeyEvents, event, playerObject->getInventoryMutable(), m_pickup->getInventoryMutable());
    if (hasKeyEvents && event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::P) {
        m_currentState = eSceneState::Playing;
    }
}

void Scene::wait(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    if (sleepTick < cRestTicks) {
        // cout << aiTick << endl;
        if (aiTick > 0.2) {
            m_currentLevel->run();
            aiTick = 0;
            sleepTick++;
            m_currentLevel->cleanup();
            cout << "sleeping" << endl;
        }
    } else {
        Log::push("You stop resting and recover all fatigue.");
        playerObject->getCreatureComponent()->resetFatigue();
        sleepTick = 0;
        m_currentState = eSceneState::Playing;
    }
}

void Scene::attack(bool hasKeyEvents, sf::Event event, PlayerObject* playerObject)
{
    if (hasKeyEvents && event.type == sf::Event::KeyReleased) {
        doMoveSelector(event, playerObject, true);
        if (event.key.code == sf::Keyboard::Enter) {
            const Object* object = m_currentLevel->getObject(m_selector.getPosition());
            if (object != nullptr) {
                if (object->getObjectType() == eObjectTypes::Creature) {
                    const CreatureObject* creatureObject
                        = static_cast<const CreatureObject*>(object);
                    if (creatureObject->isConscious() == true) {
                        playerObject->startCombatWith(creatureObject);
                        m_currentState = eSceneState::InCombat;
                    } else {
                        creatureObject->kill();
                        Log::push("You finish off the unconscious creature");
                    }
                }
            }
        }
        if (event.key.code == sf::Keyboard::A) {
            m_currentState = eSceneState::Playing;
        }
    }
}

void Scene::doMoveSelector(sf::Event event, PlayerObject* playerObject, bool limit)
{
    constexpr int cLimit = 2;
    if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::Down) {
            if (limit == false
                || m_selector.getPosition().y < playerObject->getPosition().y + cLimit) {
                m_selector.moveDown();
            }
        }
        if (event.key.code == sf::Keyboard::Up) {
            if (limit == false
                || m_selector.getPosition().y > playerObject->getPosition().y - cLimit) {
                m_selector.moveUp();
            }
        }
        if (event.key.code == sf::Keyboard::Left) {
            if (limit == false
                || m_selector.getPosition().x > playerObject->getPosition().x - cLimit) {
                m_selector.moveLeft();
            }
        }
        if (event.key.code == sf::Keyboard::Right) {
            if (limit == false
                || m_selector.getPosition().x < playerObject->getPosition().x + cLimit) {
                m_selector.moveRight();
            }
        }
    }
}
