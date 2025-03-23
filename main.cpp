#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>

#include "./client/client.h"
#include "./player/player.h"
#include "./utility/utility.h"

using namespace std;

double lastMouseX = screenW / 2.0;
double mouseOffsetX = 0.0;

void mouseCallback(GLFWwindow *window, double xpos, double ypos) {
  mouseOffsetX = xpos - lastMouseX;
  lastMouseX = xpos;
}
void centerMouse(GLFWwindow *window) {
  glfwSetCursorPos(window, screenW / 2.0, screenH / 2.0);
  lastMouseX = screenW / 2.0;
}

int main() {
  THROW_IF_FALSE(Client::getInstance().connectToServer() == true,
                 "Could not connect to server");

  unique_ptr<Player> player = make_unique<Player>(0.2);
  std::thread udpReceiverThread([&player] {
    while (player->getHealth() > 0) {
      Client::getInstance().receiveData(player.get());
    }
  });
  udpReceiverThread.detach();

  srand(time(0));
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return -1;
  }

  GLFWwindow *window =
      glfwCreateWindow(screenW, screenH, "GLFW Window", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return -1;
  }

  glfwSetWindowUserPointer(window, player.get());

  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetMouseButtonCallback(window, Player::mouseButtonCallback);
  centerMouse(window);
  glfwSetKeyCallback(window, Player::key_callback);

  glfwSetFramebufferSizeCallback(window, Utils::framebuffer_size_callback);

  while (!glfwWindowShouldClose(window)) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenW, screenH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, 1);
    }

    player->setMouseDelta(mouseOffsetX);
    mouseOffsetX = 0;
    centerMouse(window);
    glClear(GL_COLOR_BUFFER_BIT);

    player->update();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
