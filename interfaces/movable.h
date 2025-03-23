#ifndef MOVABLE_HPP
#define MOVABLE_HPP

class iMovable {
public:
    virtual ~iMovable() = default;

    virtual void move() = 0;
    virtual double& getDx() = 0;
    virtual double& getDy() = 0;
    virtual double& getSpeed() = 0;
};

#endif
