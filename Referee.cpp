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
// Vérification du contact
// =====================================

bool Referee::checkContact(
    const Ball& required,
    const Ball& touched
)
{

    /*
        Cas spécial :
        Après une rouge,
        le joueur peut choisir une couleur.
    */

    if (required.getName() == "Couleur")
    {

        // Une rouge à la place d'une couleur
        // est une faute

        if (touched.getName() == "Rouge")
        {
            m_lastFoulPoints =
                calculateFoul(
                    required,
                    touched
                );


            std::cout
                << "Faute : rouge au lieu d'une couleur - "
                << m_lastFoulPoints
                << " points"
                << std::endl;


            return false;
        }



        std::cout
            << "Contact couleur correct"
            << std::endl;


        return true;
    }



    /*
        Cas normal :
        La bille touchée doit être
        la bille demandée.
    */

    if (required.getName() == touched.getName())
    {
        std::cout
            << "Contact correct"
            << std::endl;


        return true;
    }



    // Mauvaise bille

    m_lastFoulPoints =
        calculateFoul(
            required,
            touched
        );


    std::cout
        << "Faute : "
        << m_lastFoulPoints
        << " points"
        << std::endl;


    return false;
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