#pragma once

#include "Match.h"


class GameManager
{

public:

    GameManager();


    void startMatch();


    Match& getMatch();

    void afterShot();


private:

    Match m_match;

};
