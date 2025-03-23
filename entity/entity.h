#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <string>
#include <glm/glm.hpp>

using namespace std;

class Entity {
public:
    enum class Type {
        PLAYER,
        ENEMY,
        OBJECT
    };
    const Type type;

    Entity(Type entityType);
    
    virtual void update() = 0;
    virtual void initObject() {
        this->ex = 0;
        this->ey = 0;
        this->ew = 0;
        this->eh = 0;
    }

    double getX() const { return ex; }
    double getY() const { return ey; }
    void setX(double x) { this->ex = x; }
    void setY(double y) { this->ey = y; }

    virtual void onCollision(Entity* other) {
        // ADD IMPL
    }

    glm::dvec2 getPosition() const;

    virtual ~Entity() {}

protected:
    double ex, ey, ew, eh;
};

#endif
