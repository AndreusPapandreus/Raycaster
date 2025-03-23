#pragma once
#include "GLFW/glfw3.h"
#include <cmath>

#define screenW 960
#define screenH 640
#define SENSITIVITY 0.2

#define THROW_IF_FALSE(condition, message) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error(message); \
        } \
    } while (false)

namespace Utils {

inline double degToRad(double a) { return a * M_PI / 180.0; }

inline double constrainAngle(double a) {
  if (a > 359) {
    a -= 360;
  }
  if (a < 0) {
    a += 360;
  }
  return a;
}

inline double projected_dist(double ax, double ay, double bx, double by, double ang) {
  return cos(degToRad(ang)) * (bx - ax) - sin(degToRad(ang)) * (by - ay);
}

inline void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

inline double inverse_rsqrt(double number) {
    const double threehalfs = 1.5;
    double x2 = number * 0.5;
    double y = number;

    int64_t i = *reinterpret_cast<int64_t*>(&y);
    i = 0x5fe6eb50c7b537a9 - (i >> 1);
    y = *reinterpret_cast<double*>(&i);
    y = y * (threehalfs - (x2 * y * y));

    return y;
}
} // namespace Utils
