#include "Shot.h"



Shot::Shot(const Player& player, const Ball& ball)
{
    m_playerName = player.getName();

    m_ballName = ball.getName();

    m_points = ball.getValue();
}



std::string Shot::getPlayerName() const
{
    return m_playerName;
}



std::string Shot::getBallName() const
{
    return m_ballName;
}



int Shot::getPoints() const
{
    return m_points;
}