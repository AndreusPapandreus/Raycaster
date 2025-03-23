#ifndef MAP_H
#define MAP_H

#include "../enemy/enemy.h"
#include "../utility/utility.h"
#include "GLFW/glfw3.h"
#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <SFML/Network.hpp>

#define mapX 8
#define mapY 8
#define cubeSize 64

class Map {
public:
  sf::TcpSocket tSocket;
  sf::UdpSocket udpSocket;
  unsigned short udpPort;
  sf::IpAddress serverIP;
  string username = "";
  static double fps, frame;
  static constexpr int mapSize = mapX * mapY;
  static std::array<int, mapSize> mapW;
  static std::array<int, mapSize> mapF;
  static std::array<int, mapSize> mapC;

  static Map &getInstance() {
    static Map instance;
    return instance;
  }
  static void updateFPS();
  static glm::dvec2 getRandomFreePos();

  Map(const Map &) = delete;
  Map &operator=(const Map &) = delete;

  struct Sprite {
    string name;
    Entity::Type type;
    bool alive;
    int map; // texture to show
    double x, y, z, pAngle;

    Sprite(string name = "", Entity::Type t = Entity::Type::ENEMY, bool alive = false, int m = 0,
           double xpos = 0, double ypos = 0, double zpos = 0, double pAngle = 0)
        : name(name), type(t), alive(alive), map(m), x(xpos), y(ypos), z(zpos), pAngle(pAngle) {}
  };
  static std::vector<Sprite> sp;
  static std::vector<Sprite> objects;
  static std::vector<std::reference_wrapper<Sprite>> getAllEntities() {
    std::vector<std::reference_wrapper<Sprite>> combined;
    for (auto &sprite : sp) {
      combined.push_back(sprite);
    }
    for (auto &sprite : objects) {
      combined.push_back(sprite);
    }
    return combined;
  }

private:
  Map();
};

#endif
