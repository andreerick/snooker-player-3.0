#pragma once

#include "Ball.h"

class Referee
{
public:

    Referee();


    bool checkContact(
        const Ball& required,
        const Ball& touched
    );


    int calculateFoul(
        const Ball& required,
        const Ball& touched
    );


private:

    int m_lastFoulPoints;
};