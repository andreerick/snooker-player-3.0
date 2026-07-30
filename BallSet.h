#pragma once

#include <vector>

#include "Ball.h"


class BallSet
{

public:

    BallSet();


    const std::vector<Ball>& getBalls() const;


    // Gestion des billes

    void removeBall(const std::string& name);

    void restoreBall(const Ball& ball);


    bool isOnTable(const std::string& name) const;

    int countBalls(const std::string& name) const;

private:

    std::vector<Ball> m_balls;

};
