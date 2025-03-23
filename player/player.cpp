#include "../player/player.h"
#include "../Textures/All_Textures.ppm"
#include "../Textures/damage.ppm"
#include "../Textures/hood.ppm"
#include "../Textures/hood_shot.ppm"
#include "../Textures/sky.ppm"
#include "../Textures/sprites.ppm"
#include "../client/client.h"
#include "../map/map.h"
#include "../utility/utility.h"
#include "math.h"
#include <cassert>
#include <iostream>
#include <ostream>

std::array<int, 120> depth;

void Player::key_callback(GLFWwindow *window, int key, int scancode, int action,
                          int mods) {
  Player *player = static_cast<Player *>(glfwGetWindowUserPointer(window));
  if (player) {
    player->handleKeyInput(key, action);
  }
}

void Player::mouseButtonCallback(GLFWwindow *window, int button, int action,
                                 int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    Player *player = static_cast<Player *>(glfwGetWindowUserPointer(window));
    player->isShooting = true;
    Client::getInstance().sendShootEvent(
        Map::getInstance().udpSocket, Map::getInstance().serverIP,
        Map::getInstance().udpPort, Map::getInstance().username);
    player->shootTime = glfwGetTime();
  }
}

inline Player::Direction operator|(Player::Direction a, Player::Direction b) {
  return static_cast<Player::Direction>(static_cast<int>(a) |
                                        static_cast<int>(b));
}

inline Player::Direction operator&(Player::Direction a, Player::Direction b) {
  return static_cast<Player::Direction>(static_cast<int>(a) &
                                        static_cast<int>(b));
}

inline Player::Direction operator~(Player::Direction a) {
  return static_cast<Player::Direction>(~static_cast<int>(a));
}

inline bool hasDirection(Player::Direction combo, Player::Direction dir) {
  return static_cast<uint8_t>(combo) & static_cast<uint8_t>(dir);
}

void Player::handleKeyInput(int key, int action) {
  bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
  bool released = (action == GLFW_RELEASE);

  if (key == GLFW_KEY_A) {
    if (pressed)
      dir = dir | Direction::LEFT;
    if (released)
      dir = dir & ~Direction::LEFT;
  }
  if (key == GLFW_KEY_D) {
    if (pressed)
      dir = dir | Direction::RIGHT;
    if (released)
      dir = dir & ~Direction::RIGHT;
  }
  if (key == GLFW_KEY_W) {
    if (pressed)
      dir = dir | Direction::UP;
    if (released)
      dir = dir & ~Direction::UP;
  }
  if (key == GLFW_KEY_S) {
    if (pressed)
      dir = dir | Player::Direction::DOWN;
    if (released)
      dir = dir & ~Direction::DOWN;
  }
  if (dir == static_cast<Direction>(0)) {
    dir = Direction::STOP;
  }

  // "E" key for opening doors
  if (key == GLFW_KEY_E && pressed) {
    int xo = (dx < 0) ? -25 : 25;
    int yo = (dy < 0) ? -25 : 25;
    int ipx = ex / 64.0, ipx_add_xo = (ex + xo) / 64.0;
    int ipy = ey / 64.0, ipy_add_yo = (ey + yo) / 64.0;

    if (Map::mapW[ipy_add_yo * mapX + ipx_add_xo] == 4) {
      Map::mapW[ipy_add_yo * mapX + ipx_add_xo] = 0; // Open door
    }
  }
}

Player::Player(double speed) : Entity(Entity::Type::PLAYER) {
  this->speed = speed;
}

void Player::setup() {
  glm::dvec2 pos = Map::getRandomFreePos();
  ex = pos.x;
  ey = pos.y;
  Player::setPlayerAngle(rand() % 360);
  this->dx = cos(Utils::degToRad(pAngle));
  this->dy = -sin(Utils::degToRad(pAngle));
  this->isAlive = true;
}

void Player::update() {
  Map::getInstance().updateFPS();
  if (!this->isAlive) {
    setup();
    return;
  }
  Client::getInstance().sendData(
      Map::getInstance().udpSocket, Map::getInstance().serverIP,
      Map::getInstance().udpPort, *this, Map::getInstance().username);
  if (this->health > 0) {
    if (isShooting && (glfwGetTime() - shootTime > 0.1)) {
      isShooting = false;
    }
    if (wasDamaged && (glfwGetTime() - damageTime > 1)) {
      wasDamaged = false;
    }
    move();
    drawSky();
    castRays();
    drawEnemies();
    drawHood();
  } else if (this->isAlive == true) {
    Client::getInstance().disconnect();
    this->isAlive = false;
  }
}

void Player::move() {
  double newAngle = pAngle - mouseDelta * SENSITIVITY;
  pAngle = Utils::constrainAngle(newAngle);

  dx = 0;
  dy = 0;
  if (dir == static_cast<Direction>(0)) {
    dx = 0;
    dy = 0;
    return;
  }
  if (hasDirection(dir, Direction::LEFT)) {
    dx += speed * cos(Utils::degToRad(pAngle) + M_PI / 2);
    dy += speed * -sin(Utils::degToRad(pAngle) + M_PI / 2);
  }
  if (hasDirection(dir, Direction::RIGHT)) {
    dx += speed * cos(Utils::degToRad(pAngle) - M_PI / 2);
    dy += speed * -sin(Utils::degToRad(pAngle) - M_PI / 2);
  }
  if (hasDirection(dir, Direction::UP)) {
    dx += speed * cos(Utils::degToRad(pAngle));
    dy += speed * -sin(Utils::degToRad(pAngle));
  }
  if (hasDirection(dir, Direction::DOWN)) {
    dx += -speed * cos(Utils::degToRad(pAngle));
    dy += -speed * -sin(Utils::degToRad(pAngle));
  }

  double isqrt = Utils::inverse_rsqrt(dx * dx + dy * dy);
  if (dx != 0 || dy != 0) {
    dx = (dx * isqrt) * speed;
    dy = (dy * isqrt) * speed;
  }

  int xo = (dx > 0) ? 20 : -20;
  int yo = (dy > 0) ? 20 : -20;

  int ipx = ex / cubeSize;
  int ipx_add_xo = (ex + xo) / cubeSize;
  int ipx_sub_xo = (ex - xo) / cubeSize;

  int ipy = ey / cubeSize;
  int ipy_add_yo = (ey + yo) / cubeSize;
  int ipy_sub_yo = (ey - yo) / cubeSize;

  if (ipy >= 0 && ipy < mapY && ipx_add_xo >= 0 && ipx_add_xo < mapX) {
    if (Map::mapW[ipy * mapX + ipx_add_xo] == 0) {
      ex += dx * Map::fps;
    }
  }

  if (ipy_add_yo >= 0 && ipy_add_yo < mapY && ipx >= 0 && ipx < mapX) {
    if (Map::mapW[ipy_add_yo * mapX + ipx] == 0) {
      ey += dy * Map::fps;
    }
  }
}

void Player::castRays() {
  int r, mx, my, mp, dof, side;
  float vx, vy, rx, ry, ra, xo, yo, disV, disH;

  ra = Utils::constrainAngle(pAngle + 30); // ray set back 30 degrees

  for (r = 0; r < 120; r++) {
    int vmt = 0, hmt = 0; // vertical and horizontal map texture number
    //---Vertical---
    dof = 0;
    side = 0;
    disV = 100000;
    float Tan = tan(Utils::degToRad(ra));
    if (cos(Utils::degToRad(ra)) > 0.001) {
      rx = (((int)ex >> 6) << 6) + 64;
      ry = (ex - rx) * Tan + ey;
      xo = 64;
      yo = -xo * Tan;
    } // looking left
    else if (cos(Utils::degToRad(ra)) < -0.001) {
      rx = (((int)ex >> 6) << 6) - 0.0001;
      ry = (ex - rx) * Tan + ey;
      xo = -64;
      yo = -xo * Tan;
    } // looking right
    else {
      rx = ex;
      ry = ey;
      dof = 8;
    } // looking up or down. no hit

    while (dof < 8) {
      mx = (int)(rx) >> 6;
      my = (int)(ry) >> 6;
      mp = my * mapX + mx;
      if (mp > 0 && mp < mapX * mapY && Map::mapW[mp] > 0) {
        vmt = Map::mapW[mp] - 1;
        dof = 8;
        disV = cos(Utils::degToRad(ra)) * (rx - ex) -
               sin(Utils::degToRad(ra)) * (ry - ey);
      } // hit
      else {
        rx += xo;
        ry += yo;
        dof += 1;
      } // check next horizontal
    }
    vx = rx;
    vy = ry;

    //---Horizontal---
    dof = 0;
    disH = 100000;
    Tan = 1.0 / Tan;
    if (sin(Utils::degToRad(ra)) > 0.001) {
      ry = (((int)ey >> 6) << 6) - 0.0001;
      rx = (ey - ry) * Tan + ex;
      yo = -64;
      xo = -yo * Tan;
    } // looking up
    else if (sin(Utils::degToRad(ra)) < -0.001) {
      ry = (((int)ey >> 6) << 6) + 64;
      rx = (ey - ry) * Tan + ex;
      yo = 64;
      xo = -yo * Tan;
    } // looking down
    else {
      rx = ex;
      ry = ey;
      dof = 8;
    } // looking straight left or right

    while (dof < 8) {
      mx = (int)(rx) >> 6;
      my = (int)(ry) >> 6;
      mp = my * mapX + mx;
      if (mp > 0 && mp < mapX * mapY && Map::mapW[mp] > 0) {
        hmt = Map::mapW[mp] - 1;
        dof = 8;
        disH = cos(Utils::degToRad(ra)) * (rx - ex) -
               sin(Utils::degToRad(ra)) * (ry - ey);
      } // hit
      else {
        rx += xo;
        ry += yo;
        dof += 1;
      } // check next horizontal
    }

    float shade = 1;
    glColor3f(0, 0.8, 0);
    if (disV < disH) {
      hmt = vmt;
      shade = 0.5;
      rx = vx;
      ry = vy;
      disH = disV;
      glColor3f(0, 0.6, 0);
    } // horizontal hit first

    int ca = Utils::constrainAngle(pAngle - ra);
    disH = disH * cos(Utils::degToRad(ca)); // fix fisheye
    int lineH = (cubeSize * 640) / (disH);
    float ty_step = 32.0 / (float)lineH;
    float ty_off = 0;
    if (lineH > 640) {
      ty_off = (lineH - 640) / 2.0;
      lineH = 640;
    } // line height and limit
    int lineOff = 320 - (lineH >> 1); // line offset

    depth[r] = disH; // save this line's depth
    //---draw walls---
    int y;
    float ty = ty_off * ty_step; //+hmt*32;
    float tx;
    if (shade == 1) {
      tx = (int)(rx / 2.0) % 32;
      if (ra > 180) {
        tx = 31 - tx;
      }
    } else {
      tx = (int)(ry / 2.0) % 32;
      if (ra > 90 && ra < 270) {
        tx = 31 - tx;
      }
    }
    for (y = 0; y < lineH; y++) {
      int pixel = ((int)ty * 32 + (int)tx) * 3 + (hmt * 32 * 32 * 3);
      int red = All_Textures[pixel + 0] * shade;
      int green = All_Textures[pixel + 1] * shade;
      int blue = All_Textures[pixel + 2] * shade;
      glPointSize(16);
      glColor3ub(red, green, blue);
      glBegin(GL_POINTS);
      glVertex2i(r * 8, y + lineOff);
      glEnd();
      ty += ty_step;
    }

    //---draw floors---
    for (y = lineOff + lineH; y < 640; y++) {
      double dy = y - (640 / 2.0), deg = Utils::degToRad(ra),
             raFix = cos(Utils::degToRad(Utils::constrainAngle(pAngle - ra)));
      tx = ex / 2.0 + cos(deg) * 158 * 2 * 32 / dy / raFix;
      ty = ey / 2.0 - sin(deg) * 158 * 2 * 32 / dy / raFix;

      int index = (int)(ty / 32.0) * mapX + (int)(tx / 32.0);
      int mp = Map::mapF[(int)(ty / 32.0) * mapX + (int)(tx / 32.0)] * 32 * 32;
      int pixel = (((int)(ty) & 31) * 32 + ((int)(tx) & 31)) * 3 + mp * 3;
      int red = All_Textures[pixel + 0] * 0.7;
      int green = All_Textures[pixel + 1] * 0.7;
      int blue = All_Textures[pixel + 2] * 0.7;
      glPointSize(16);
      glColor3ub(red, green, blue);
      glBegin(GL_POINTS);
      glVertex2i(r * 8, y);
      glEnd();

      //---draw ceiling---
      mp = Map::mapC[(int)(ty / 32.0) * mapX + (int)(tx / 32.0)] * 32 * 32;
      pixel = (((int)(ty) & 31) * 32 + ((int)(tx) & 31)) * 3 + mp * 3;
      red = All_Textures[pixel + 0];
      green = All_Textures[pixel + 1];
      blue = All_Textures[pixel + 2];
      if (mp > 0) {
        glPointSize(16);
        glColor3ub(red, green, blue);
        glBegin(GL_POINTS);
        glVertex2i(r * 8, 640 - y);
        glEnd();
      }
    }

    ra = Utils::constrainAngle(ra - 0.5); // go to next ray, 60 total
  }
}

void Player::drawSky() const {
  int x, y;
  for (y = 0; y < 40; y++) {
    for (x = 0; x < 120; x++) {
      int xo = (int)pAngle * 2 - x;
      if (xo < 0) {
        xo += 120;
      }
      xo = xo % 120; // return 0-120 based on player angle
      int pixel = (y * 120 + xo) * 3;
      int red = sky[pixel + 0];
      int green = sky[pixel + 1];
      int blue = sky[pixel + 2];
      glPointSize(16);
      glColor3ub(red, green, blue);
      glBegin(GL_POINTS);
      glVertex2i(x * 8, y * 8);
      glEnd();
    }
  }
}

void Player::drawHood() const {
  float frequency = 42.0f;
  float amplitude = 8.0f;
  int offset =
      static_cast<int>(std::sin(Map::frame * 0.2 * frequency) * amplitude);
  for (int y = 0; y < 80; y++) {
    for (int x = 0; x < 120; x++) {
      int pixel = (y * 120 + x) * 3;
      int *texture = isShooting ? hood_shot : hood_texture;
      int red = texture[pixel + 0];
      int green = texture[pixel + 1];
      int blue = texture[pixel + 2];
      if (red != 255 && blue != 255) {
        glPointSize(16);
        glColor3ub(red, green, blue);
        glBegin(GL_POINTS);
        if (dir != Player::Direction::STOP) {
          glVertex2i(x * 8, y * 8 + offset + 9);
        } else {
          glVertex2i(x * 8, y * 8 + 5);
        }
        glEnd();
      }
    }
  }
  if (wasDamaged) {
    for (int y = 0; y < 80; y++) {
      for (int x = 0; x < 120; x++) {
        int pixel = (y * 120 + x) * 3;
        int red = damage_texture[pixel + 0];
        int green = damage_texture[pixel + 1];
        int blue = damage_texture[pixel + 2];
        if (red != 255 && blue != 255) {
          glPointSize(16);
          glColor3ub(red, green, blue);
          glBegin(GL_POINTS);
          glVertex2i(x * 8, y * 8);
          glEnd();
        }
      }
    }
  }
}

void Player::setPosition(double x, double y) {
  assert(x < mapX * cubeSize && x > 0 && y < mapY * cubeSize && y > 0);
  this->ex = x;
  this->ey = y;
}

void Player::setPlayerAngle(double newAngle) {
  assert(newAngle >= 0 && newAngle < 360);
  this->pAngle = newAngle;
}

void Player::setMouseDelta(double offset) { this->mouseDelta = offset; }

double &Player::getDx() { return this->dx; }
double &Player::getDy() { return this->dy; }
double &Player::getSpeed() { return this->speed; }

void Player::drawEnemies() const {
  int x, y;

  for (auto &entity : Map::getAllEntities()) {
    if (!entity.get().alive)
      continue; // Skip if entity is not alive

    if (entity.get().type == Type::OBJECT && ex < entity.get().x + 30 &&
        ex > entity.get().x - 30 && ey < entity.get().y + 30 &&
        ey > entity.get().y - 30) {
      entity.get().alive = false;
      glfwTerminate();
    } // hit bomb

    // --- Animate Sprite ---
    static double lastFrame = 0;
    if (Map::frame - lastFrame >= 0.2) {
      if (entity.get().type != Entity::Type::OBJECT) {
        entity.get().map ^= 1; // Toggle map index between 0 and 1
        lastFrame = Map::frame;
      }
    }

    // --- Player Collision with Object ---
    if (ex < entity.get().x + 30 && ex > entity.get().x - 30 &&
        ey < entity.get().y + 30 && ey > entity.get().y - 30) {
      std::cout << "ENTITY INTERACTION" << std::endl;
    }

    // --- Enemy Movement Logic ---
    // int spx = (int)entity.get().x >> 6, spy = (int)entity.get().y >> 6;
    // int spx_add = ((int)entity.get().x + 15) >> 6,
    //     spy_add = ((int)entity.get().y + 15) >> 6;
    // int spx_sub = ((int)entity.get().x - 15) >> 6,
    //     spy_sub = ((int)entity.get().y - 15) >> 6;
    //
    // if (entity.get().type == Type::ENEMY) {
    //   if (entity.get().x > ex && Map::mapW[spy * 8 + spx_sub] == 0) {
    //     entity.get().x -= 0.04 * Map::fps;
    //   }
    //   if (entity.get().x < ex && Map::mapW[spy * 8 + spx_add] == 0) {
    //     entity.get().x += 0.04 * Map::fps;
    //   }
    //   if (entity.get().y > ey && Map::mapW[spy_sub * 8 + spx] == 0) {
    //     entity.get().y -= 0.04 * Map::fps;
    //   }
    //   if (entity.get().y < ey && Map::mapW[spy_add * 8 + spx] == 0) {
    //     entity.get().y += 0.04 * Map::fps;
    //   }
    // }

    // --- Convert World Coordinates to Screen Space ---
    float sx = entity.get().x - ex;
    float sy = entity.get().y - ey;
    float sz = entity.get().z;

    float CS = cos(Utils::degToRad(pAngle)), SN = sin(Utils::degToRad(pAngle));
    float a = sy * CS + sx * SN;
    float b = sx * CS - sy * SN;
    sx = a;
    sy = b;

    sx = (sx * 108.0 / sy) + (120 / 2.0);
    sy = (sz * 108.0 / sy) + (80 / 2.0);

    int scale = 32 * 80 / b;
    if (scale < 0)
      scale = 0;
    if (scale > 120)
      scale = 120;

    // --- Texture Mapping ---
    float t_x = 0, t_y = 31, t_x_step = 31.5 / (float)scale,
          t_y_step = 32.0 / (float)scale;

    for (x = sx - scale / 2.0; x < sx + scale / 2.0; x++) {
      t_y = 31;
      for (y = 0; y < scale; y++) {
        if (entity.get().alive && x > 0 && x < 120 && b < depth[x]) {
          int pixel =
              ((int)t_y * 32 + (int)t_x) * 3 + (entity.get().map * 32 * 32 * 3);
          int red = sprites[pixel + 0];
          int green = sprites[pixel + 1];
          int blue = sprites[pixel + 2];

          if (red != 255 && green != 0 && blue != 255) { // Exclude purple color
            glPointSize(16);
            glColor3ub(red, green, blue);
            glBegin(GL_POINTS);
            glVertex2i(x * 8, sy * 8 - y * 8);
            glEnd();
          }
          t_y -= t_y_step;
          if (t_y < 0)
            t_y = 0;
        }
      }
      t_x += t_x_step;
    }
  }
}

void Player::makeDamage(int decrease) {
  this->health -= decrease;
  this->wasDamaged = true;
  this->damageTime = glfwGetTime();
}

double Player::getAngle() const { return this->pAngle; }
int Player::getHealth() const { return this->health; }
