#include "./client.h"
#include <thread>
#include <unordered_set>

sf::Packet &operator>>(sf::Packet &packet, PlayerDTO &dto) {
  int statusInt;
  packet >> statusInt >> dto.name >> dto.xPos >> dto.yPos >> dto.pAngle;
  dto.status = static_cast<PlayerDTO::Status>(statusInt);
  return packet;
}

bool Client::connectToServer() const {
  string serverIp = "";
  unsigned short port;
  std::cout << "Enter server IP: ";
  getline(cin, serverIp);
  std::cout << "Enter server port: ";
  std::cin >> port;

  // auto tSocket = std::make_shared<sf::TcpSocket>();

  sf::Socket::Status status = Map::getInstance().tSocket.connect(serverIp, port);
  if (status != sf::Socket::Done) {
    std::cout << "Error while connecting to server" << std::endl;
  } else {
    Map::getInstance().serverIP = Map::getInstance().tSocket.getRemoteAddress();
    std::cout << "Connection established" << std::endl;

    if (Map::getInstance().udpSocket.bind(sf::Socket::AnyPort) !=
        sf::Socket::Done) {
      std::cout << "Error while binding udp socket" << std::endl;
    }
    const unsigned short udpPort = Map::getInstance().udpSocket.getLocalPort();
    cout << "My UdpPort: " << udpPort << endl;

    string myPassword = "";
    string udpPortStr = std::to_string(udpPort);
    std::cout << "Enter your name: ";
    std::cin >> Map::getInstance().username;
    std::cout << "Enter your password: ";
    std::cin >> myPassword;
    string data =
        Map::getInstance().username + "," + myPassword + "," + udpPortStr;
    THROW_IF_FALSE(Map::getInstance().tSocket.send(data.c_str(), data.size() + 1) ==
                       sf::Socket::Done,
                   "Could not send data");
  }

  char logResp[32];
  size_t received_1;
  THROW_IF_FALSE(Map::getInstance().tSocket.receive(logResp, sizeof(logResp), received_1) ==
                     sf::Socket::Done,
                 "Did not get response from server");

  string buffer(logResp, received_1);
  string result = "";
  unsigned short serverUdpPort = 0;
  size_t delimiterPos = buffer.find(",");

  if (delimiterPos != std::string::npos) {
    result = buffer.substr(0, delimiterPos);
    string serverUdpPortStr = buffer.substr(delimiterPos + 1);
    serverUdpPort = stoul(serverUdpPortStr);
  }
  cout << "Server Udp Port: " << serverUdpPort << endl;
  Map::getInstance().udpPort = serverUdpPort;

  std::thread tcpKeepAliveThread([] {
    while (true) {
      sf::Packet pingPacket;
      pingPacket << std::string("PING");

      if (Map::getInstance().tSocket.send(pingPacket) != sf::Socket::Done) {
        std::cerr
            << "[PING] Failed to send ping packet — possibly disconnected."
            << std::endl;
        break; // exit loop if sending fails
      }

      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  });
  tcpKeepAliveThread.detach();

  return true;
}

bool Client::sendData(sf::UdpSocket &socket, const sf::IpAddress serverIp,
                      const unsigned short serverPort, const Player &player,
                      const std::string &clientName) const {
  sf::Packet packet;
  packet << "POS" << clientName << player.getX() << player.getY()
         << player.getAngle();

  if (socket.send(packet, serverIp, serverPort) == sf::Socket::Done) {
    return true;
  }

  std::cerr << "Error sending data to server!" << std::endl;
  return false;
}

bool Client::sendShootEvent(sf::UdpSocket &socket, const sf::IpAddress serverIp,
                            const unsigned short serverPort,
                            const string &clientName) const {
  sf::Packet packet;
  packet << "SHOOT" << clientName;
  if (socket.send(packet, serverIp, serverPort) == sf::Socket::Done) {
    return true;
  }
  std::cerr << "Error sending data to server!" << std::endl;
  return false;
}

void Client::receiveData(Player *player) const {
  sf::Packet packet;
  sf::IpAddress senderIp;
  unsigned short senderPort;

  if (Map::getInstance().udpSocket.receive(packet, senderIp, senderPort) !=
      sf::Socket::Done) {
    std::cerr << "Failed to receive player data!" << std::endl;
  }

  std::string tag;
  if (!(packet >> tag)) {
    std::cerr << "Malformed packet: missing tag" << std::endl;
    return;
  }

  if (tag == "DAMAGED") {
    player->makeDamage(1);
  } else if (tag == "DEFAULT") {
    std::uint32_t count;
    packet >> count;

    std::unordered_set<std::string> receivedNames;

    for (std::uint32_t i = 0; i < count; ++i) {
      PlayerDTO dto;
      packet >> dto;
      receivedNames.insert(dto.name);

      auto it = std::find_if(
          Map::sp.begin(), Map::sp.end(),
          [&](const Map::Sprite &s) { return s.name == dto.name; });

      if (it != Map::sp.end()) {
        it->x = dto.xPos;
        it->y = dto.yPos;
        it->pAngle = dto.pAngle;
        it->alive = (dto.status == PlayerDTO::Status::Online);
      } else {
        Map::Sprite newSprite(dto.name, Entity::Type::ENEMY,
                              dto.status == PlayerDTO::Status::Online, 2,
                              dto.xPos, dto.yPos, 20, dto.pAngle);
        Map::sp.push_back(newSprite);
      }
    }

    Map::sp.erase(std::remove_if(Map::sp.begin(), Map::sp.end(),
                                 [&](const Map::Sprite &s) {
                                   return receivedNames.find(s.name) ==
                                          receivedNames.end();
                                 }),
                  Map::sp.end());
  }
}

void Client::disconnect() const {
  sf::Packet packet;
  packet << "DISCONNECT" << Map::getInstance().username;
  Map::getInstance().udpSocket.send(packet, Map::getInstance().serverIP,
                                    Map::getInstance().udpPort);
  Map::getInstance().udpSocket.unbind();
  Map::getInstance().tSocket.disconnect();
  std::cout << "Disconnected from server." << std::endl;
}
