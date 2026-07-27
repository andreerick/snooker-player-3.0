#pragma once

#include <vector>
#include "Ball.h"


class BallSet
{

public:

    BallSet();


    const std::vector<Ball>& getBalls() const;


private:

    std::vector<Ball> m_balls;

};