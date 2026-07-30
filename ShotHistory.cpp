#include "ShotHistory.h"

#include <iostream>


// =====================================
// Constructeur
// =====================================

ShotHistory::ShotHistory()
{

}


// =====================================
// Ajouter un coup
// =====================================

void ShotHistory::addShot(
    const Shot& shot
)
{
    m_shots.push_back(shot);
}


// =====================================
// Ajouter une faute
// =====================================

void ShotHistory::addFoul(
    const std::string& playerName,
    const std::string& requiredBall,
    const std::string& touchedBall,
    const std::string& reason,
    int points
)
{
    FoulRecord foul;

    foul.playerName = playerName;

    foul.requiredBall = requiredBall;

    foul.touchedBall = touchedBall;

    foul.reason = reason;

    foul.points = points;


    m_fouls.push_back(foul);
}


// =====================================
// Nombre de coups
// =====================================

int ShotHistory::getShotCount() const
{
    return static_cast<int>(m_shots.size());
}


// =====================================
// Affichage historique
// =====================================

void ShotHistory::displayHistory() const
{
    std::cout
        << "=== HISTORIQUE DES COUPS ==="
        << std::endl;


    for (size_t i = 0; i < m_shots.size(); i++)
    {
        std::cout
            << "Coup "
            << i + 1
            << " : "
            << m_shots[i].getPlayerName()
            << " - "
            << m_shots[i].getBallName()
            << " - "
            << m_shots[i].getPoints()
            << " points"
            << std::endl;
    }


    if (!m_fouls.empty())
    {
        std::cout
            << std::endl
            << "=== FAUTES ==="
            << std::endl;


        for (const auto& foul : m_fouls)
        {
            std::cout
                << "FAUTE : "
                << foul.playerName
                << std::endl;


            std::cout
                << "Bille demandee : "
                << foul.requiredBall
                << std::endl;


            std::cout
                << "Bille jouee : "
                << foul.touchedBall
                << std::endl;


            std::cout
                << "Motif : "
                << foul.reason
                << std::endl;


            std::cout
                << "Penalite : "
                << foul.points
                << " points"
                << std::endl;
        }
    }
}