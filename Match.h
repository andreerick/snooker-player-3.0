#pragma once

#include "Player.h"
#include "Frame.h"


class Match
{

public:

    Match();

    void start();

    Player& getPlayer1();

    Player& getPlayer2();


    Frame& getCurrentFrame();


    void startNewFrame();


    void frameWon(Player& player);


    int getFramesPlayer1() const;

    int getFramesPlayer2() const;


    bool isMatchFinished() const;



private:

    Player m_player1;

    Player m_player2;


    Frame m_currentFrame;


    int m_framesPlayer1;

    int m_framesPlayer2;


    int m_framesToWin;

};