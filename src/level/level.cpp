#include "level.h"
#include "../3rdparty/random.hpp"
#include "../object/campfireobject.h"
#include "../object/corpseobject.h"
#include "../object/creatureobject.h"
#include "../object/humanobject.h"
#include "../scene.h"

using namespace effolkronium;
using namespace std;

Level::Level(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_data(width * height)
    , m_lighting(eLighting::Cave)
    , m_logic(eLevelLogic::None)
{
}

Level::~Level()
{
    cleanup();
    clearObjects();
}

void Level::save() {}

void Level::load() {}

void Level::run(Scene* scene)
{
    switch (m_logic) {
    case eLevelLogic::Arena: {
        // temporary, should really be scripted somewhere or something
        // but then it would become a real engine and thats a big problem
        int count = 0;
        for (unsigned i = 0; i < m_objects.size(); ++i) {
            Object* object = m_objects[i];
            if (object->getObjectType() == eObjectTypes::Creature) {
                count++;
            }
        }
        if (count < 2) {
            generateEnemy();
        }

    } break;

    default:
        break;
    }
    // OOP should be replaced with ECS if possible
    for (unsigned i = 0; i < m_objects.size(); ++i) {
        if (m_objects[i]->deleteMe() == true) {
            Object* object = m_objects[i];
            if (object->getObjectType() == eObjectTypes::Creature) {
                CreatureObject* creatureObject = static_cast<CreatureObject*>(object);
                CorpseObject* corpse
                    = new CorpseObject(creatureObject->getCreatureComponent()->getName());
                corpse->setPosition(creatureObject->getPosition());
                corpse->setInventory(creatureObject->getInventory());
                m_objects.push_back(corpse);
            }
            m_toDelete.push_back(object);
            m_objects.erase(m_objects.begin() + i);
        }
        m_objects[i]->run(this);
        vector2d pos = m_objects[i]->getPosition();
        Tile tile = (*this)(pos.x, pos.y);
        if (tile.m_levelChangeIdx != -1) {
            // change level
            // idx is coupled to scene, as scene manages all levels
            // so this is unavoidable
            scene->changeToLevel(tile.m_levelChangeIdx, m_objects[i], 1, 1);
        }
    }

    for (auto trigger : m_globalTriggers) {
        if (trigger->run(scene) == false) {
            // delete
        }
    }
}

void Level::generateEnemy()
{
    HumanObject* object = new HumanObject;
    object->setPosition(5, 5);
    object->setFaction(eCreatureFaction::ArenaFighter);
    // temporary
    object->setLoadout(eCreatureFaction::Confederacy,
        static_cast<eRank>(random_static::get((int)eRank::Recruit, (int)eRank::Count - 1)));
    object->getCreatureComponent()->setAgility(random_static::get(5, 9));
    object->getCreatureComponent()->setIntuition(random_static::get(5, 9));
    object->getCreatureComponent()->setProficiency(
        eWeaponTypes::Polearms, random_static::get(8, 12));
    object->getCreatureComponent()->setProficiency(
        eWeaponTypes::Brawling, random_static::get(8, 12));
    object->getCreatureComponent()->setProficiency(
        eWeaponTypes::Longswords, random_static::get(8, 12));
    object->getCreatureComponent()->setProficiency(eWeaponTypes::Swords, random_static::get(8, 12));
    object->getCreatureComponent()->setProficiency(eWeaponTypes::Mass, random_static::get(8, 12));
    addObject(object);
}

void Level::generate()
{
    std::vector<Room> rooms;

    for (int x = 1; x < m_width; ++x) {
        for (int y = 1; y < m_height; ++y) {
            (*this)(x, y).m_type = eTileType::Wall;
        }
    }

    rooms.push_back(carveRoom(1, 1, 6, 6, 10, 10));

    for (int i = 0; i < 9; ++i) {
        rooms.push_back(carveRoom());
        int idx = rooms.size() - 1;
        createCorridor(rooms[idx - 1], rooms[idx]);
    }
    for (int i = 0; i < 5; ++i) {
        rooms.push_back(carveSeperateRoom());
        int idx = rooms.size() - 1;
        createCorridor(rooms[idx - 1], rooms[idx]);
    }

    // makeRoom();
    removeIslands();

    // temporary, to add enemies
    for (unsigned i = 1; i < rooms.size(); ++i) {
        if (random_static::get(1, 2) == 1) {
            HumanObject* object = new HumanObject;
            object->setPosition(rooms[i].x, rooms[i].y);
            object->setFaction(eCreatureFaction::Bandit);
            object->setLoadout(eCreatureFaction::Bandit, eRank::Recruit);
            m_objects.push_back(object);
        }
    }

    // add enemy to farthest room as a boss
    int minDist = 0;
    int idx = 0;
    for (unsigned i = 1; i < rooms.size(); ++i) {
        int a = rooms[i].x - rooms[0].x;
        int b = rooms[i].y - rooms[0].y;

        int dist = (a * a) + (b * b);
        if (dist > minDist) {
            minDist = dist;
            idx = i;
        }
    }
    for (unsigned i = rooms[idx].x; i < m_width; ++i) {
        (*this)(i, rooms[idx].y).m_type = eTileType::Ground;
    }
    HumanObject* object = new HumanObject;
    object->setPosition(rooms[idx].x, rooms[idx].y);
    object->setFaction(eCreatureFaction::Bandit);
    object->setLoadout(eCreatureFaction::Bandit, eRank::Veteran);
    object->getCreatureComponent()->setAgility(9);
    object->getCreatureComponent()->setIntuition(9);
    object->setName("One-Eyed Doyt");
    m_objects.push_back(object);
}

void Level::generateTown()
{
    m_lighting = eLighting::Sunny;
    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_height; ++y) {
            (*this)(x, y).m_material = eTileMaterial::Grass;
            (*this)(x, y).m_type = eTileType::Ground;
        }
    }

    // generate town square
    constexpr int cSquareSize = 4;
    for (int x = m_width / 2 - cSquareSize; x < m_width / 2 + cSquareSize; ++x) {
        for (int y = m_height / 2 - cSquareSize; y < m_height / 2 + cSquareSize; ++y) {
            (*this)(x, y).m_material = eTileMaterial::Stone;
            (*this)(x, y).m_type = eTileType::Ground;
        }
    }

    for (unsigned i = 0; i < 12; ++i) {
        createBuilding();
    }
}

Level::Building Level::createBuilding()
{
    constexpr int cMax = 12;
    constexpr int cMin = 8;
    bool canBuild = false;
    int xlen, ylen, xStart, yStart;
    Building building;
    // make sure there is no buildings that already exist first
    while (canBuild == false) {
        xlen = random_static::get(cMin, cMax);
        ylen = random_static::get(cMin, cMax);
        xStart = random_static::get(2, m_width - xlen - 1);
        yStart = random_static::get(2, m_height - ylen - 1);
        bool foundWall = false;
        for (int x = xStart - 1; x <= xStart + xlen; ++x) {
            for (int y = yStart - 1; y <= yStart + ylen; ++y) {
                if ((*this)(x, y).m_material == eTileMaterial::Stone) {
                    foundWall = true;
                }
            }
        }
        if (foundWall == false) {
            canBuild = true;
        }
    }

    for (int x = xStart; x < xStart + xlen; ++x) {
        for (int y = yStart; y < yStart + ylen; ++y) {
            if (x == xStart || x == xStart + xlen - 1 || y == yStart || y == yStart + ylen - 1) {
                (*this)(x, y).m_type = eTileType::Wall;
            } else {
                (*this)(x, y).m_type = eTileType::Ground;
            }

            (*this)(x, y).m_material = eTileMaterial::Stone;
        }
    }
    building.width = xlen;
    building.height = ylen;
    building.x = xStart;
    building.y = yStart;
    if (random_static::get(1, 2) == 1) {
        int xDoor = random_static::get(xStart + 1, xStart + xlen - 2);
        (*this)(xDoor, yStart + ylen - 1).m_type = eTileType::Ground;
        building.xDoor = xDoor;
        building.yDoor = yStart + ylen - 1;
    } else {
        int yDoor = random_static::get(yStart + 1, yStart + ylen - 2);
        (*this)(xStart, yDoor).m_type = eTileType::Ground;
        building.xDoor = xStart;
        building.yDoor = yDoor;
    }
    return building;
}

Level::Room Level::carveRoom()
{
    constexpr int max = 10;
    int xlen = random_static::get(4, max);
    int ylen = random_static::get(4, max);
    int xStart = random_static::get(2, m_width - max - 1);
    int yStart = random_static::get(2, m_height - max - 1);

    for (int x = xStart; x < xStart + xlen; ++x) {
        for (int y = yStart; y < yStart + ylen; ++y) {
            (*this)(x, y).m_type = eTileType::Ground;
        }
    }

    return { random_static::get(xStart + 1, xlen + xStart - 1),
        random_static::get(yStart + 1, ylen + yStart - 1) };
}

Level::Room Level::carveRoom(
    int xStart, int yStart, int minSizeX, int minSizeY, int maxSizeX, int maxSizeY)
{
    int xlen = random_static::get(minSizeX, maxSizeX);
    int ylen = random_static::get(minSizeY, maxSizeY);

    for (int x = xStart; x < xStart + xlen; ++x) {
        for (int y = yStart; y < yStart + ylen; ++y) {
            (*this)(x, y).m_type = eTileType::Ground;
        }
    }

    return { random_static::get(xStart + 1, xlen + xStart - 1),
        random_static::get(yStart + 1, ylen + yStart - 1) };
}

Level::Room Level::carveSeperateRoom()
{
    constexpr int cMax = 10;
    int size = cMax;
    int xlen, ylen, xStart, yStart;

    bool canPlace = false;
    unsigned iteration = 0;
    while (canPlace == false) {
        if (iteration > 10000) {
            // lower our standards
            size = max(size - 1, 2);
        }
        bool hasGround = false;
        xlen = random_static::get(2, size);
        ylen = random_static::get(2, size);
        xStart = random_static::get(3, m_width - size - 4);
        yStart = random_static::get(3, m_height - size - 4);

        for (int x = xStart - 2; x < xStart + xlen + 2; ++x) {
            for (int y = yStart - 2; y < yStart + ylen + 2; ++y) {
                if ((*this)(x, y).m_type == eTileType::Ground) {
                    hasGround = true;
                }
            }
        }
        canPlace = !hasGround;
        iteration++;
        cout << "Iteration: " << iteration << endl;
    }

    for (int x = xStart; x < xStart + xlen; ++x) {
        for (int y = yStart; y < yStart + ylen; ++y) {
            (*this)(x, y).m_type = eTileType::Ground;
        }
    }
    return { random_static::get(xStart + 1, xlen + xStart - 1),
        random_static::get(yStart + 1, ylen + yStart - 1) };
}

void Level::createCorridor(Room room1, Room room2)
{
    int x = 0;
    if (room1.x < room2.x) {
        for (x = room1.x; x < room2.x; ++x) {
            (*this)(x, room1.y).m_type = eTileType::Ground;
        }
    } else {
        for (x = room1.x; x >= room2.x; --x) {
            (*this)(x, room1.y).m_type = eTileType::Ground;
        }
    }

    if (room1.y < room2.y) {
        for (int y = room1.y; y < room2.y; ++y) {
            (*this)(x, y).m_type = eTileType::Ground;
        }
    } else {
        for (int y = room1.y; y >= room2.y; --y) {
            (*this)(x, y).m_type = eTileType::Ground;
        }
    }
}

void Level::makeRoom()
{
    bool canPlace = false;
    int xlen;
    int ylen;
    int xStart;
    int yStart;
    while (canPlace == false) {
        xlen = random_static::get(4, 10);
        ylen = random_static::get(4, 10);
        xStart = random_static::get(2, 20);
        yStart = random_static::get(2, 20);

        // check for existing room
        bool hasWall = false;
        for (int x = xStart - 2; x < xlen + xStart + 2; x++) {
            if ((*this)(x, yStart).m_type == eTileType::Wall) {
                hasWall = true;
                break;
            }
        }
        for (int y = yStart - 2; y < ylen + yStart + 2; y++) {
            if ((*this)(xStart, y).m_type == eTileType::Wall) {
                hasWall = true;
                break;
            }
        }
        for (int x = xStart - 2; x < xlen + xStart + 2; x++) {
            if ((*this)(x, yStart + ylen).m_type == eTileType::Wall) {
                hasWall = true;
                break;
            }
        }
        for (int y = yStart - 2; y < ylen + yStart + 2; y++) {
            if ((*this)(xStart + xlen, y).m_type == eTileType::Wall) {
                hasWall = true;
                break;
            }
        }

        canPlace = !hasWall;
    }

    for (int x = xStart; x < xlen + xStart; x++) {
        (*this)(x, yStart).m_type = eTileType::Wall;
    }
    for (int y = yStart; y < ylen + yStart; y++) {
        (*this)(xStart, y).m_type = eTileType::Wall;
    }

    for (int x = xStart; x < xlen + xStart; x++) {
        (*this)(x, yStart + ylen).m_type = eTileType::Wall;
    }

    for (int y = yStart; y < ylen + yStart + 1; y++) {
        (*this)(xStart + xlen, y).m_type = eTileType::Wall;
    }

    int dir = effolkronium::random_static::get(1, 4);
    int xDoor = effolkronium::random_static::get(xStart + 1, xlen + xStart - 1);
    int yDoor = effolkronium::random_static::get(yStart + 2, ylen + yStart - 2);
    switch (dir) {
    case 1:
        (*this)(xDoor, yStart).m_type = eTileType::Ground;
        break;
    case 2:
        (*this)(xStart, yDoor).m_type = eTileType::Ground;
        break;
    case 3:
        (*this)(xDoor, yStart + ylen).m_type = eTileType::Ground;
        break;
    case 4:
        (*this)(xStart + xlen, yDoor).m_type = eTileType::Ground;
        break;
    }
}

void Level::removeIslands()
{
    for (int x = 1; x < m_width - 2; ++x) {
        for (int y = 1; y < m_height - 2; ++y) {
            if ((*this)(x, y).m_type == eTileType::Wall) {
                bool single = true;
                if ((*this)(x - 1, y).m_type == eTileType::Wall) {
                    single = false;
                }
                if ((*this)(x + 1, y).m_type == eTileType::Wall) {
                    single = false;
                }
                if ((*this)(x, y - 1).m_type == eTileType::Wall) {
                    single = false;
                }
                if ((*this)(x, y + 1).m_type == eTileType::Wall) {
                    single = false;
                }

                if (single == true) {
                    (*this)(x, y).m_type = eTileType::Ground;
                }
            }
        }
    }
}

void Level::cleanup()
{
    for (unsigned i = 0; i < m_toDelete.size(); ++i) {
        cout << "clearing.." << endl;
        delete m_toDelete[i];
    }
    m_toDelete.clear();
}

void Level::clearObjects()
{
    for (unsigned i = 0; i < m_objects.size(); ++i) {
        if (m_objects[i]->preserveBetweenLevels() == false) {
            delete m_objects[i];
        }
    };
    m_objects.clear();
}

const Object* Level::getObject(vector2d position)
{
    for (unsigned i = 0; i < m_objects.size(); ++i) {
        vector2d objectPosition = m_objects[i]->getPosition();
        if (position.x == objectPosition.x && position.y == objectPosition.y) {
            return m_objects[i];
        }
    }
    return nullptr;
}

Object* Level::removeObject(Object::ObjectId id)
{
    for (unsigned i = 0; i < m_objects.size(); ++i) {
        if (m_objects[i]->getId() == id) {
            m_objects.erase(m_objects.begin() + i);
            return m_objects[i];
        }
    }
    return nullptr;
}

Object* Level::getObjectMutable(vector2d position, const Object* exclude)
{
    for (unsigned i = 0; i < m_objects.size(); ++i) {
        vector2d objectPosition = m_objects[i]->getPosition();
        if (position.x == objectPosition.x && position.y == objectPosition.y
            && exclude != m_objects[i]) {
            return m_objects[i];
        }
    }
    return nullptr;
}

bool Level::isFreeSpace(int x, int y) const
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) {
        return false;
    }
    if ((*this)(x, y).m_type == eTileType::Wall) {
        return false;
    }
    for (unsigned i = 0; i < m_objects.size(); ++i) {
        vector2d position = m_objects[i]->getPosition();
        if (position.x == x && position.y == y && m_objects[i]->hasCollision() == true) {
            return false;
        }
    }
    return true;
}

std::vector<Object*> Level::getObjectsAtLocation(vector2d position)
{
    std::vector<Object*> ret;
    for (unsigned i = 0; i < m_objects.size(); ++i) {
        vector2d objectPosition = m_objects[i]->getPosition();
        if (position.x == objectPosition.x && position.y == objectPosition.y) {
            ret.push_back(m_objects[i]);
        }
    }

    return ret;
}
