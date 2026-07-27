#include "BallSet.h"


BallSet::BallSet()
{
    // 15 billes rouges
    for (int i = 0; i < 15; i++)
    {
        m_balls.push_back(Ball("Rouge", 1));
    }


    // Couleurs
    m_balls.push_back(Ball("Jaune", 2));

    m_balls.push_back(Ball("Verte", 3));

    m_balls.push_back(Ball("Marron", 4));

    m_balls.push_back(Ball("Bleue", 5));

    m_balls.push_back(Ball("Rose", 6));

    m_balls.push_back(Ball("Noire", 7));


    // Bille blanche
    m_balls.push_back(Ball("Blanche", 0));
}



const std::vector<Ball>& BallSet::getBalls() const
{
    return m_balls;
}