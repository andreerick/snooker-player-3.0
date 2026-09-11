#include "Referee.h"

#include <iostream>



// =====================================
// Constructeur
// =====================================

Referee::Referee()
{
    m_lastFoulPoints = 0;
}



// =====================================
// Calcul de la faute
// =====================================

int Referee::calculateFoul(
    const Ball& required,
    const Ball& touched
)
{
    int points = 4;



    // Valeur de la bille demandée

    if (required.getValue() > points)
    {
        points = required.getValue();
    }



    // Valeur de la bille touchée

    if (touched.getValue() > points)
    {
        points = touched.getValue();
    }



    // Maximum snooker : 7 points

    if (points > 7)
    {
        points = 7;
    }



    m_lastFoulPoints = points;


    return points;
}



// =====================================
// Dernière faute
// =====================================

int Referee::getLastFoulPoints() const
{
    return m_lastFoulPoints;
}