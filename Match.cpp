#include "Match.h"
#include <iostream>


Match::Match()
{
    m_framesPlayer1 = 0;

    m_framesPlayer2 = 0;


    // Par défaut : match en 3 frames gagnantes
    m_framesToWin = 2;
}

void Match::start()
{
    std::cout << "=== Debut du match ==="
        << std::endl;

    startNewFrame();
}



Player& Match::getPlayer1()
{
    return m_player1;
}



Player& Match::getPlayer2()
{
    return m_player2;
}




Frame& Match::getCurrentFrame()
{
    return m_currentFrame;
}





void Match::startNewFrame()
{
    m_currentFrame = Frame();
}





void Match::frameWon(Player& player)
{
    if (&player == &m_player1)
    {
        m_framesPlayer1++;
    }
    else
    {
        m_framesPlayer2++;
    }
}





int Match::getFramesPlayer1() const
{
    return m_framesPlayer1;
}





int Match::getFramesPlayer2() const
{
    return m_framesPlayer2;
}





bool Match::isMatchFinished() const
{
    if (m_framesPlayer1 >= m_framesToWin)
    {
        return true;
    }


    if (m_framesPlayer2 >= m_framesToWin)
    {
        return true;
    }


    return false;
}