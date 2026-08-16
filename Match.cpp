#include "Match.h"
#include <iostream>


Match::Match()
{
    m_framesPlayer1 = 0;

    m_framesPlayer2 = 0;


    // Par défaut : match en 3 frames gagnantes
    m_framesToWin = 2;

    m_player1.setName("Joueur 1");
    m_player2.setName("Joueur 2");
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
    m_currentFrame.setPlayerNames(m_player1.getName(), m_player2.getName());
}

void Match::setPlayerNames(const std::string& name1, const std::string& name2)
{
    m_player1.setName(name1);
    m_player2.setName(name2);
}

void Match::checkFrameEnd()
{
    if (!m_currentFrame.isFinished())
    {
        return;
    }

    std::string winnerName = m_currentFrame.getWinnerName();

    if (winnerName == m_player1.getName())
    {
        frameWon(m_player1);
    }
    else if (winnerName == m_player2.getName())
    {
        frameWon(m_player2);
    }
    else
    {
        std::cout
            << "Frame terminee sur une egalite - cas non gere (noire a rejouer)"
            << std::endl;
        return;
    }

    if (!isMatchFinished())
    {
        startNewFrame();
    }
    else
    {
        std::cout << "=== Match termine ===" << std::endl;
    }
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
