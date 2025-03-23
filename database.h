#ifndef DATABASE_H
#define DATABASE_H

#include <SFML/Network.hpp>
#include <algorithm>
#include <iostream>
#include <unordered_map>

using namespace std;

struct Player {
  shared_ptr<sf::TcpSocket> tSocket;
  shared_ptr<sf::UdpSocket> uSocket;
  unsigned short uPort;
  enum class Status { Online, Offline };
  Status status;
  string name;
  string passwd;
  sf::IpAddress ipaddress;
  double xPos;
  double yPos;
  double pAngle;
  bool wasDamaged = false;
  Player() : status(Status::Offline) {}
};

struct PlayerDTO {
  enum class Status { Online, Offline };
  Status status;
  std::string name;
  double xPos;
  double yPos;
  double pAngle;

  PlayerDTO() : xPos(0), yPos(0), pAngle(0), status(Status::Offline) {}
  PlayerDTO(const Player &player) : status(static_cast<Status>(player.status)), name(player.name), xPos(player.xPos), yPos(player.yPos), pAngle(player.pAngle) {}
};
unordered_map<std::string, Player> players;

#endif
