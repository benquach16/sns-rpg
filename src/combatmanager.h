#pragma once

#include <set>
#include <string>
#include <vector>

#include "combatinstance.h"
#include "creatures/creature.h"
#include "object/creatureobject.h"

enum class eCombatManagerState { RunCombat, PositioningRoll };

class CombatEdge {
public:
    typedef unsigned EdgeId;
    CombatEdge(CombatInstance* instance, CombatManager* vertex1, CombatManager* vertex2);
    CombatInstance* getInstance() const { return m_instance; }
    CombatManager* getVertex1() const { return m_vertex1; }
    CombatManager* getVertex2() const { return m_vertex2; }
    void remove();
    EdgeId getId() { return m_id; }
    void setActive(bool active) { m_active = active; }
    bool getActive() const { return m_active; }

private:
    EdgeId m_id;
    CombatEdge();
    bool m_active;
    CombatInstance* m_instance;
    CombatManager* m_vertex1;
    CombatManager* m_vertex2;
};

class CombatNode {
public:
    CombatNode(CreatureObject* creature);
    void run();

private:
    CombatNode();
    std::vector<CombatEdge> m_edges;
    CreatureObject* m_creature;
};

// node for a graph. instances act as edges
class CombatManager {
public:
    static constexpr float cTick = 0.8;
    static constexpr int cMaxEngaged = 5;
    CombatManager(CreatureObject* creature);
    ~CombatManager();
    bool run(float tick);
    CombatInstance* getCurrentInstance() const;
    void startCombatWith(const CreatureObject* creature);
    eCombatManagerState getState() const { return m_currentState; }
    bool isEngaged() const { return m_edges.size() > 0; }
    void addEdge(CombatEdge edge);
    bool canEngage() const { return m_edges.size() < cMaxEngaged; }
    void remove(CombatEdge::EdgeId id);

private:
    CombatManager();
    enum class eOutnumberedSide { None, Side1, Side2 };
    void doRunCombat(float tick);
    void doPositionRoll();
    void cleanup();
    void switchInitiative()
    {
        m_currentTempo = (m_currentTempo == eTempo::First) ? eTempo::Second : eTempo::First;
    }
    void refreshInstances();
    void writeMessage(
        const std::string& str, Log::eMessageTypes type = Log::eMessageTypes::Standard);

    std::vector<CombatEdge> m_edges;
    unsigned m_edgeId;
    bool m_positionDone;
    bool m_doPositionRoll;
    CreatureObject* m_mainCreature;
    bool m_isPlayers;
    eOutnumberedSide m_side;
    eTempo m_currentTempo;
    eCombatManagerState m_currentState;
};
