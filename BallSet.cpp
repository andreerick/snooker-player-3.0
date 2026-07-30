#include "BallSet.h"


BallSet::BallSet()
{
    // 15 billes rouges

    for (int i = 0; i < 15; i++)
    {
        m_balls.push_back(
            Ball("Rouge", 1)
        );
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



// =====================================
// Liste des billes
// =====================================

const std::vector<Ball>& BallSet::getBalls() const
{
    return m_balls;
}



// =====================================
// Retirer une bille de la table
// =====================================

void BallSet::removeBall(
    const std::string& name
)
{
    for (auto it = m_balls.begin(); it != m_balls.end(); ++it)
    {
        if (it->getName() == name)
        {
            m_balls.erase(it);
            return;
        }
    }
}



// =====================================
// Remettre une bille sur la table
// =====================================

void BallSet::restoreBall(
    const Ball& ball
)
{
    m_balls.push_back(ball);
}



// =====================================
// Vérifier présence sur table
// =====================================

bool BallSet::isOnTable(
    const std::string& name
) const
{
    for (const auto& ball : m_balls)
    {
        if (ball.getName() == name)
        {
            return true;
        }
    }


    return false;
}

// =====================================
// Compter les billes d'un type
// =====================================

int BallSet::countBalls(
    const std::string& name
) const
{
    int count = 0;


    for (const auto& ball : m_balls)
    {
        if (ball.getName() == name)
        {
            count++;
        }
    }


    return count;
}