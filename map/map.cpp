#include "./map.h"

std::vector<Map::Sprite> Map::sp;
std::vector<Map::Sprite> Map::objects;

Map::Map() {
  // sp[0].type = Entity::Type::ENEMY;
  // sp[0].alive = true;
  // sp[0].map = 2;
  // sp[0].x = 2.5 * 64;
  // sp[0].y = 2 * 64;
  // sp[0].z = 20; // enemy
  //
  // sp[1].type = Entity::Type::ENEMY;
  // sp[1].alive = true;
  // sp[1].map = 2;
  // sp[1].x = 3.5 * 64;
  // sp[1].y = 4.5 * 64;
  // sp[1].z = 20; // enemy

  Sprite bomb = {
    "",
    Entity::Type::OBJECT,
    true,
    0,
    2.5 * 64,
    4.5 * 64,
    20,
  };
  objects.push_back(bomb);
}

std::array<int, Map::mapSize> Map::mapW = {
    1, 1, 1, 1, 2, 2, 2, 2,
    1, 0, 0, 1, 0, 0, 0, 2,
    1, 0, 0, 4, 0, 6, 0, 2,
    1, 5, 2, 5, 0, 0, 0, 2,
    2, 0, 0, 0, 0, 0, 0, 1,
    2, 0, 0, 0, 0, 1, 0, 1,
    2, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1
};

std::array<int, Map::mapSize> Map::mapF = {
    0, 2, 1, 0, 8, 0, 1, 2,
    6, 0, 0, 0, 0, 1, 2, 7,
    0, 8, 0, 0, 2, 0, 8, 1,
    1, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 0, 0, 6, 2, 8, 0,
    1, 0, 0, 1, 7, 8, 0, 0,
    2, 0, 8, 2, 0, 0, 1, 1,
    2, 0, 8, 0, 0, 0, 6, 0
};

std::array<int, Map::mapSize> Map::mapC = {
    0, 0, 2, 0, 4, 0, 0, 0,
    0, 4, 4, 4, 4, 2, 2, 0,
    0, 4, 4, 4, 2, 0, 2, 0,
    0, 4, 0, 0, 2, 2, 2, 4,
    0, 0, 0, 0, 0, 2, 4, 0,
    4, 0, 0, 0, 2, 4, 4, 0,
    0, 0, 0, 0, 2, 4, 4, 0,
    0, 0, 2, 0, 0, 4, 2, 0
};

double Map::fps = 0.0;
double Map::frame = 0.0;

glm::dvec2 Map::getRandomFreePos() {
  int x, y, index;
  do {
    x = 1 + rand() % (mapX - 2);
    y = 1 + rand() % (mapY - 2);
    index = y * mapX + x;
  } while (Map::mapW[index] != 0);

  return glm::dvec2(x * cubeSize + cubeSize / 2, y * cubeSize + cubeSize / 2);
}

void Map::updateFPS() {
  double frame2 = glfwGetTime();    // Get time since program start
  fps = (frame2 - frame) * 1000.0; // Convert seconds to milliseconds
  frame = glfwGetTime();
}

