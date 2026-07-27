#pragma once

#include "Match.h"


class GameManager
{

public:

    GameManager();


    void startMatch();


    Match& getMatch();



private:

    Match m_match;

};