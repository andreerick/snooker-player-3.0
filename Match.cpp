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

void Match::setFramesToWin(int frames)
{
    if (frames > 0)
    {
        m_framesToWin = frames;
    }
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

    // Le joueur qui "casse" (commence) alterne a chaque frame : frame 1 =
    // joueur 1, frame 2 = joueur 2, frame 3 = joueur 1, etc. Sans ceci,
    // Frame::Frame() met TOUJOURS m_currentPlayer sur le joueur 1 par
    // defaut, quelle que soit la frame (bug signale par l'utilisateur :
    // le joueur 1 commencait systematiquement chaque frame).
    int framesPlayed = m_framesPlayer1 + m_framesPlayer2;
    if (framesPlayed % 2 == 1)
    {
        m_currentFrame.switchPlayer();
    }
}

void Match::setPlayerNames(const std::string& name1, const std::string& name2)
{
    m_player1.setName(name1);
    m_player2.setName(name2);
}

void Match::checkFrameEnd()
{
    // Le match est deja termine : ne plus jamais recompter de victoire
    // de frame. Sans ce garde, appeler cette fonction plusieurs fois sur
    // la MEME frame deja terminee (ex. bouton "Fin de frame" clique a
    // nouveau apres la fin du match, ou m_awaitingNextFrame remis a
    // false par proceedToNextFrame() sans jamais demarrer une vraie
    // nouvelle frame une fois le match fini) incrementait
    // m_framesPlayer1/2 indefiniment au-dela de m_framesToWin (bug
    // observe : "7 | 0" affiche sur un match en 3 frames gagnantes).
    if (isMatchFinished())
    {
        return;
    }

    if (!m_currentFrame.isFinished())
    {
        return;
    }

    // Deja traitee : on attend que proceedToNextFrame() soit appele
    // (laisse le temps a l'UI d'afficher le score final de la frame).
    if (m_awaitingNextFrame)
    {
        return;
    }

    std::string winnerName = m_currentFrame.getWinnerName();

    if (winnerName == m_player1.getName())
    {
        m_frameResults.push_back(
            { winnerName, m_currentFrame.getPlayer1().getScore(), m_currentFrame.getPlayer2().getScore() }
        );
        frameWon(m_player1);
    }
    else if (winnerName == m_player2.getName())
    {
        m_frameResults.push_back(
            { winnerName, m_currentFrame.getPlayer1().getScore(), m_currentFrame.getPlayer2().getScore() }
        );
        frameWon(m_player2);
    }
    else
    {
        std::cout
            << "Frame terminee sur une egalite - cas non gere (noire a rejouer)"
            << std::endl;
        return;
    }

    m_awaitingNextFrame = true;

    if (isMatchFinished())
    {
        std::cout << "=== Match termine ===" << std::endl;
    }
}

bool Match::isFrameJustFinished() const
{
    return m_awaitingNextFrame;
}

void Match::undoFrameConclusion()
{
    if (!m_awaitingNextFrame)
    {
        return;
    }

    if (!m_frameResults.empty())
    {
        const FrameResult& last = m_frameResults.back();
        if (last.winnerName == m_player1.getName())
        {
            --m_framesPlayer1;
        }
        else if (last.winnerName == m_player2.getName())
        {
            --m_framesPlayer2;
        }
        m_frameResults.pop_back();
    }

    m_awaitingNextFrame = false;
}

void Match::proceedToNextFrame()
{
    if (!m_awaitingNextFrame)
    {
        return;
    }

    m_awaitingNextFrame = false;

    if (!isMatchFinished())
    {
        startNewFrame();
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

int Match::getFramesToWin() const
{
    return m_framesToWin;
}

const std::vector<FrameResult>& Match::getFrameResults() const
{
    return m_frameResults;
}