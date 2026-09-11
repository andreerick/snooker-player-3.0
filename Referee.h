#pragma once

#include "Ball.h"


class Referee
{

public:

    // Constructeur
    Referee();



    // Calcul de la pénalité
    int calculateFoul(
        const Ball& required,
        const Ball& touched
    );



    // Dernière faute enregistrée
    int getLastFoulPoints() const;



private:

    int m_lastFoulPoints;

};