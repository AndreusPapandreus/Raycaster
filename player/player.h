#ifndef PLAYER_H
#define PLAYER_H

#include "../entity/entity.h"
#include "../interfaces/movable.h"
#include "GLFW/glfw3.h"
#include <vector>
#include <SFML/Network.hpp>

class Player : public Entity, iMovable {
public:
  sf::UdpSocket udpSocket;
  unsigned short udpPort;
  sf::IpAddress serverIpAddress;
  enum class Direction : uint8_t {
    STOP = 0,
    LEFT = 1 << 0,
    RIGHT = 1 << 1,
    UP = 1 << 2,
    DOWN = 1 << 3
  };

  Player(double speed);

  void update() override;
  void setup();
  void move() override;
  double &getDx() override;
  double &getDy() override;
  double &getSpeed() override;
  void castRays();
  void setPosition(double x, double y);
  void setPlayerAngle(double newAngle);
  double getAngle() const;
  int getHealth() const;
  void setMouseDelta(double offset);
  void makeDamage(int decrease);
  void drawSky() const;
  void drawHood() const;
  void drawEnemies() const;
  void handleKeyInput(int key, int action);
  static void key_callback(GLFWwindow *window, int key, int scancode,
                           int action, int mods);
  static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

protected:
  Direction dir = Direction::STOP;
  double dx = 0, dy = 0, speed = 0, pAngle = 0, mouseDelta = 0, shootTime = 0, damageTime = 0;
  bool isAlive = false, isShooting = false, wasDamaged = false;
  int health = 5;
};

#endif
