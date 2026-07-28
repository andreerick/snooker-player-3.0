#pragma once

#include "Match.h"
#include "Tournament.h"


class GameManager
{

public:

    GameManager();


    void startMatch();


    Match& getMatch();

    const Tournament& getTournament() const;


private:

    Match m_match;

    Tournament m_tournament;


    void initializeTournament();

    Ball chooseBallForSimulation(const Frame& frame, int shotIndex) const;

    void displayStatus(const Frame& frame, int shotNumber) const;

};