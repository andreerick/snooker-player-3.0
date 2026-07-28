#include "Referee.h"

#include <iostream>


Referee::Referee()
{
    m_lastFoulPoints = 0;
}



bool Referee::checkContact(
    const Ball& required,
    const Ball& touched
)
{
    // Quand une couleur quelconque est requise, toute bille couleur est valide
    if (required.getName() == "Couleur")
    {
        const std::string& n = touched.getName();
        bool isColor = (n == "Jaune" || n == "Vert"   || n == "Marron" ||
                        n == "Bleu"  || n == "Rose"   || n == "Noir");

        if (isColor)
        {
            std::cout
                << "Contact correct : "
                << n
                << std::endl;

            return true;
        }

        m_lastFoulPoints = calculateFoul(required, touched);

        std::cout
            << "Faute : couleur requise, "
            << n
            << " touchee - "
            << m_lastFoulPoints
            << " points"
            << std::endl;

        return false;
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


    // Quand une couleur est requise (valeur 0), la penalite est
    // la valeur de la bille touchee, minimum 4
    if (required.getName() != "Couleur" &&
        required.getValue() > points)
    {
        points = required.getValue();
    }


    if (points < 4)
    {
        points = 4;
    }


    return points;
}
