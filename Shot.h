#pragma once

#include <string>
#include "Player.h"
#include "Ball.h"


class Shot
{

public:

    Shot(
        const Player& player,
        const Ball& ball
    );


    std::string getPlayerName() const;

    std::string getBallName() const;

    int getPoints() const;


private:

    std::string m_playerName;

    std::string m_ballName;

    int m_points;

};