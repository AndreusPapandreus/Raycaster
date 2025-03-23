#include "entity.h"

using namespace std;

Entity::Entity(Type entityType)
    : type(entityType) {
  initObject();
}

glm::dvec2 Entity::getPosition() const { return glm::dvec2(this->ex, this->ey); }
