#include "database.h"
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;
sf::Mutex mutex_1;
std::mutex playersMutex;
short numberOfThreads = 0;

sf::Packet &operator<<(sf::Packet &packet, const PlayerDTO &player) {
  packet << static_cast<int>(player.status) << player.name << player.xPos
         << player.yPos << player.pAngle;
  return packet;
}

sf::Packet &operator>>(sf::Packet &packet, PlayerDTO &player) {
  int statusInt;
  packet >> statusInt >> player.name >> player.xPos >> player.yPos >>
      player.pAngle;
  player.status = static_cast<PlayerDTO::Status>(statusInt);
  return packet;
}

struct Vec2 {
  double x, y;
};

struct Box {
  Vec2 min;
  Vec2 max;
};

struct Ray {
  Vec2 origin;
  Vec2 dir;    // normalized direction
  Vec2 invDir; // computed as 1/dir (with clamping)
};

double safeInverse(double d) {
  const double epsilon = 1e-8;
  return (fabs(d) < epsilon) ? 1e8 : 1.0 / d;
}

bool intersection(Box b, Ray r) {
  double tx1 = (b.min.x - r.origin.x) * r.invDir.x;
  double tx2 = (b.max.x - r.origin.x) * r.invDir.x;

  double tmin = std::min(tx1, tx2);
  double tmax = std::max(tx1, tx2);

  double ty1 = (b.min.y - r.origin.y) * r.invDir.y;
  double ty2 = (b.max.y - r.origin.y) * r.invDir.y;

  tmin = std::max(tmin, std::min(ty1, ty2));
  tmax = std::min(tmax, std::max(ty1, ty2));

  // Ensure intersection is in front of the ray
  if (tmax < 0)
    return false;

  return tmax >= tmin;
}

void checkShooting(double x, double y, double angle,
                   const std::string &username) {
  double angleRad = angle * M_PI / 180.0;

  // Compute normalized direction
  Vec2 dir = {cos(angleRad), sin(angleRad)};
  // Compute inverse direction safely:
  Vec2 invDir = {safeInverse(dir.x), safeInverse(dir.y)};

  Ray ray = {{x, y}, dir, invDir};

  for (auto &[name, p] : players) {
    if (name == username)
      continue;

    // Adjust half-size if needed (e.g., 16 for 32x32 box)
    Box playerBox = {
        {p.xPos - 16, 640 - (p.yPos - 16)}, // convert to Cartesian system
        {p.xPos + 16, 640 - (p.yPos + 16)}};

    if (intersection(playerBox, ray)) {
      std::cout << "[HIT] Player: " << name << "\n";
      p.wasDamaged = true;
    }
  }
}

void handleTcpClient(shared_ptr<sf::TcpSocket> client,
                     const string &clientName) {
  while (true) {
    // check for disconnections
    sf::Packet packet;
    sf::Socket::Status status = client->receive(packet);
    if (status == sf::Socket::Disconnected) {
      mutex_1.lock();
      auto it = players.find(clientName);
      if (it != players.end()) {
        players[clientName].status = Player::Status::Offline;
        numberOfThreads--;
        cout << "Client disconnected: " << client->getRemoteAddress()
             << endl; // couts last associated address
        cout << "Number of threads: " << numberOfThreads << endl;
        client->disconnect();
      }
      mutex_1.unlock();
      return;
    }

    // clean up corpses
    std::lock_guard<std::mutex> lock(playersMutex);
    for (auto it = players.begin(); it != players.end();) {
      if (it->second.status == Player::Status::Offline) {
        it = players.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void launchTcpThread(shared_ptr<sf::TcpSocket> client,
                     const string &clientName) {
  thread t(&handleTcpClient, std::move(client), clientName);
  t.detach();
  cout << "tcp thread launched" << endl;
  numberOfThreads++;
  cout << "number of tcp threads: " << numberOfThreads << endl;
}

void handleUdpClient(shared_ptr<sf::UdpSocket> udpSocket,
                     const string &clientName,
                     const unsigned short &userUdpPort) {

  sf::Packet receivePacket;
  sf::IpAddress senderIp;
  unsigned short senderPort;

  while (true) {
    if (udpSocket->receive(receivePacket, senderIp, senderPort) ==
        sf::Socket::Done) {

      std::string tag;
      if (!(receivePacket >> tag)) {
        std::cerr << "[UDP] Malformed packet (no tag)" << std::endl;
        return;
      }

      std::string receivedName;

      if (tag == "POS") {
        double xPos = 0.0, yPos = 0.0, pAngle = 0.0;
        if (receivePacket >> receivedName >> xPos >> yPos >> pAngle) {

          {
            std::lock_guard<std::mutex> lock(playersMutex);
            auto it = players.find(receivedName);
            if (it != players.end()) {
              it->second.xPos = xPos;
              it->second.yPos = yPos;
              it->second.pAngle = pAngle;
            }
          }

          sf::Packet packet;
          packet << "DEFAULT"
                 << static_cast<std::uint32_t>(players.size() -
                                               1); // Send number of players

          {
            std::lock_guard<std::mutex> lock(playersMutex);
            for (auto &[name, player] : players) {
              if (name == clientName) {
                if (player.wasDamaged) {
                  sf::Packet damagePacket;
                  damagePacket << "DAMAGED";
                  udpSocket->send(damagePacket, senderIp, senderPort);
                  player.wasDamaged = false;
                }
                continue;
              }
              PlayerDTO dto(player);
              packet << dto;
            }
          }

          udpSocket->send(packet, senderIp, senderPort);
        }
      } else if (tag == "SHOOT") {
        if (receivePacket >> receivedName) {
          const auto it = players.find(receivedName);
          checkShooting(it->second.xPos, 640 - it->second.yPos,
                        it->second.pAngle, receivedName);
        }
      } else if (tag == "DISCONNECT") {
        std::lock_guard<std::mutex> lock(playersMutex);
        auto it = players.find(clientName);
        if (it != players.end()) {
          it->second.status = Player::Status::Offline;
        }
      } else {
        std::cerr << "[UDP] Unknown packet tag: " << tag << std::endl;
      }
    } else {
      std::lock_guard<std::mutex> lock(playersMutex);
      auto it = players.find(clientName);
      if (it != players.end()) {
        it->second.status = Player::Status::Offline;
      }
    }

    // // clean up corpses
    // std::lock_guard<std::mutex> lock(playersMutex);
    // for (auto it = players.begin(); it != players.end();) {
    //   if (it->second.status == Player::Status::Offline) {
    //     it = players.erase(it);
    //   } else {
    //     ++it;
    //   }
    // }
  }
}

void launchUdpThread(shared_ptr<sf::UdpSocket> udpSocket,
                     const string &clientName, const unsigned short &udpPort) {
  thread u(&handleUdpClient, std::move(udpSocket), clientName, udpPort);
  u.detach();
  cout << "UDP thread launched" << endl;
}

int main() {
  sf::TcpListener listener;
  if (listener.listen(0) != sf::Socket::Done) {
    cout << "Error when listening to connection" << endl;
    return 1;
  }

  cout << "Server has been launched on port " << listener.getLocalPort()
       << endl;
  vector<thread> threads;
  const unsigned short udpBasePort = 12345;

  while (true) {
    string clientName = "", clientPassword = "";
    unsigned short udpPort = 0;

    shared_ptr<sf::TcpSocket> client = make_shared<sf::TcpSocket>();
    shared_ptr<sf::UdpSocket> udpSocket = make_shared<sf::UdpSocket>();

    if (udpSocket->bind(sf::Socket::AnyPort) != sf::Socket::Done) {
      cout << "Error binding the UDP port" << endl;
      return 1;
    }

    cout << "Waiting for new TCP connections..." << endl;

    if (listener.accept(*client) == sf::Socket::Done) {
      cout << "New TCP connection accepted" << endl;
      char buffer[1024];
      size_t received;
      if (client->receive(buffer, sizeof(buffer), received) ==
          sf::Socket::Done) {
        string data(buffer, received);
        size_t firstDelimiterPos = data.find(",");
        size_t secondDelimiterPos = data.find(",", firstDelimiterPos + 1);

        if (firstDelimiterPos != std::string::npos &&
            secondDelimiterPos != std::string::npos) {
          clientName = data.substr(0, firstDelimiterPos);
          clientPassword =
              data.substr(firstDelimiterPos + 1,
                          secondDelimiterPos - firstDelimiterPos - 1);
          string udpPortStr = data.substr(secondDelimiterPos + 1);
          try {
            udpPort = stoul(udpPortStr);
          } catch (const std::invalid_argument &ia) {
            cerr << "Invalid argument: " << ia.what() << std::endl;
          } catch (const std::out_of_range &oor) {
            cerr << "Out of range: " << oor.what() << std::endl;
          }
        }
        // Check for unique name and correct password(also send a server
        // udpPort)
        string valid = "SUCC," + to_string(udpSocket->getLocalPort());
        string invalid = "FAIL";

        // validation
        auto it = players.find(clientName);

        if (it == players.end()) {
          players[clientName].status = Player::Status::Online;
          players[clientName].passwd = clientPassword;
          players[clientName].ipaddress = client->getRemoteAddress();
          players[clientName].tSocket = client;
          players[clientName].uSocket = udpSocket;
          players[clientName].uPort = udpPort;

          assert(client->send(valid.c_str(), valid.size() + 1) ==
                 sf::Socket::Done);

          threads.emplace_back(&launchTcpThread, client, clientName);
          threads.emplace_back(&launchUdpThread, udpSocket, clientName,
                               udpPort);

        } else {
          auto &player = it->second;

          if (player.passwd == clientPassword &&
              player.status == Player::Status::Offline) {
            player.status = Player::Status::Online;
            player.ipaddress = client->getRemoteAddress();
            player.tSocket = client;
            player.uSocket = udpSocket;
            player.uPort = udpPort;

            assert(client->send(valid.c_str(), valid.size() + 1) ==
                   sf::Socket::Done);

            threads.emplace_back(&launchTcpThread, client, clientName);
            threads.emplace_back(&launchUdpThread, udpSocket, clientName,
                                 udpPort);
          } else {
            // Wrong password or already online
            if (client->send(invalid.c_str(), invalid.size() + 1) !=
                sf::Socket::Done) {
              std::cerr << "Error sending validation response" << std::endl;
            }

            std::cout << "Player with such credentials is: "
                      << (player.status == Player::Status::Online ? "Online"
                                                                  : "Offline")
                      << std::endl;
          }
        }
      } else {
        cout << "Error receiving client's data" << endl;
      }
    }
  }

  for (auto &t : threads) {
    t.join();
  }
  return 0;
}
