#ifndef CLIENT_H
#define CLIENT_H

#include "../map/map.h"
#include "../utility/utility.h"
#include "../player/player.h"
#include <SFML/Network.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>

using namespace std;

struct PlayerDTO {
  enum class Status { Online, Offline };
  Status status;
  std::string name;
  double xPos;
  double yPos;
  double pAngle;
};

class Client {
public:
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;

  static Client &getInstance() {
    static Client instance;
    return instance;
  }

  bool connectToServer() const;
  bool sendData(sf::UdpSocket &socket, const sf::IpAddress serverIp,
                const unsigned short serverPort, const Player &player, const string &clientName) const;
  bool sendShootEvent(sf::UdpSocket &socket, const sf::IpAddress serverIp, const unsigned short serverPort, const string &clientName) const;
  void receiveData(Player* player) const;
  void disconnect() const ;

private:
  sf::TcpSocket socket;
  std::mutex mutex_;

  Client() = default;

  ~Client() { disconnect(); }
};

#endif
