#include "ShotHistory.h"
#include <iostream>


ShotHistory::ShotHistory()
{

}


void ShotHistory::addShot(const Shot& shot)
{
    m_shots.push_back(shot);
}


int ShotHistory::getShotCount() const
{
    return static_cast<int>(m_shots.size());
}


void ShotHistory::displayHistory() const
{
    std::cout << "=== HISTORIQUE DES COUPS ===" << std::endl;


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
}