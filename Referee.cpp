#include "Referee.h"

#include <iostream>

namespace
{
constexpr const char* kNomBilleCouleur = "Couleur";
constexpr const char* kNomBilleRouge = "Rouge";
constexpr const char* kNomBilleBlanche = "Blanche";
}


Referee::Referee()
{
    m_lastFoulPoints = 0;
}



bool Referee::checkContact(
    const Ball& required,
    const Ball& touched
)
{
    if (required.getName() == kNomBilleCouleur)
    {
        if (
            touched.getName() != kNomBilleRouge
            && touched.getName() != kNomBilleBlanche
            && touched.getValue() >= 2
        )
        {
            std::cout
                << "Contact correct (couleur)"
                << std::endl;

            return true;
        }
    }


    if (required.getName() == touched.getName())
    {
        std::cout
            << "Contact correct"
            << std::endl;

        return true;
    }


    m_lastFoulPoints =
        calculateFoul(required, touched);


    std::cout
        << "Faute : "
        << m_lastFoulPoints
        << " points"
        << std::endl;


    return false;
}



int Referee::calculateFoul(
    const Ball& required,
    const Ball& touched
)
{
    int points = touched.getValue();


    if (required.getValue() > points)
    {
        points = required.getValue();
    }


    if (points < 4)
    {
        points = 4;
    }


    return points;
}