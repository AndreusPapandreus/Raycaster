#ifndef ENEMY_H
#define ENEMY_H

#include "../entity/entity.h"

class Enemy: public Entity {
public:
    Enemy();
    void update() override;
};

#endif
