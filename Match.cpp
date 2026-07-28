#include "Match.h"
#include <iostream>


Match::Match()
{
    m_player1.setName("Joueur 1");
    m_player2.setName("Joueur 2");

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

    m_currentFrame.getPlayer1().setName(m_player1.getName());
    m_currentFrame.getPlayer2().setName(m_player2.getName());
}





void Match::frameWon(Player& player)
{
    if (player.getName() == m_player1.getName())
    {
        m_framesPlayer1++;
    }
    else if (player.getName() == m_player2.getName())
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