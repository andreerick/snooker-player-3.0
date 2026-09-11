#include "Shot.h"

#include <iostream>



// =====================================
// Constructeur
// =====================================

Shot::Shot(
    const Player& player,
    const Ball& ball
)
{
    
        
    m_playerName =
        player.getName();

        
    m_ballName =
        ball.getName();


    m_points =
        ball.getValue();


    
}



// =====================================
// Informations du joueur
// =====================================

std::string Shot::getPlayerName() const
{
    return m_playerName;
}



// =====================================
// Informations de la bille
// =====================================

std::string Shot::getBallName() const
{
    return m_ballName;
}



// =====================================
// Points du coup
// =====================================

int Shot::getPoints() const
{
    return m_points;
}