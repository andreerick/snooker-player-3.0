#include "GameManager.h"

#include <iostream>

namespace
{
// Garde-fou pour garantir la fin de la simulation même en cas de bug logique.
constexpr int kMaxSimulationShots = 200;
// Injection d'une faute toutes les N tentatives pour valider l'arbitrage.
constexpr int kFoulFrequencyShots = 10;
}


GameManager::GameManager()
{

}



void GameManager::startMatch()
{
    m_match.start();

    initializeTournament();

    Frame& frame = m_match.getCurrentFrame();

    for (int shotIndex = 0; shotIndex < kMaxSimulationShots; shotIndex++)
    {
        if (frame.isFinished())
        {
            break;
        }

        displayStatus(frame, shotIndex + 1);

        const Ball ball = chooseBallForSimulation(frame, shotIndex);

        std::cout
            << "Coup joue : "
            << ball.getName()
            << " ("
            << ball.getValue()
            << " pts)"
            << std::endl;

        frame.playTurn(ball);
    }

    std::cout << "=== FRAME TERMINE ===" << std::endl;

    frame.getHistory().displayHistory();

    const int score1 = frame.getPlayer1().getScore();
    const int score2 = frame.getPlayer2().getScore();

    std::cout
        << frame.getPlayer1().getName()
        << " : "
        << score1
        << " | "
        << frame.getPlayer2().getName()
        << " : "
        << score2
        << std::endl;

    if (score1 > score2)
    {
        m_match.frameWon(m_match.getPlayer1());
    }
    else if (score2 > score1)
    {
        m_match.frameWon(m_match.getPlayer2());
    }
    else
    {
        std::cout
            << "Frame nulle, aucun point de frame attribue"
            << std::endl;
    }

    std::cout
        << "Score match (frames) : "
        << m_match.getPlayer1().getName()
        << " "
        << m_match.getFramesPlayer1()
        << " - "
        << m_match.getFramesPlayer2()
        << " "
        << m_match.getPlayer2().getName()
        << std::endl;
}



Match& GameManager::getMatch()
{
    return m_match;
}


const Tournament& GameManager::getTournament() const
{
    return m_tournament;
}


void GameManager::initializeTournament()
{
    m_tournament = Tournament();

    m_tournament.addPlayer(m_match.getPlayer1());
    m_tournament.addPlayer(m_match.getPlayer2());

    std::cout
        << "Tournoi initialise avec "
        << m_tournament.getPlayerCount()
        << " joueurs"
        << std::endl;
}


Ball GameManager::chooseBallForSimulation(const Frame& frame, int shotIndex) const
{
    // Injection périodique d'une faute pour exercer la logique d'arbitrage.
    if (shotIndex > 0 && shotIndex % kFoulFrequencyShots == 0)
    {
        return Ball("Blanche", 0);
    }

    const Ball required = frame.getRequiredBall();

    if (required.getName() == "Couleur")
    {
        return Ball("Noire", 7);
    }

    return required;
}


void GameManager::displayStatus(const Frame& frame, int shotNumber) const
{
    std::cout
        << "\n--- Coup "
        << shotNumber
        << " ---"
        << std::endl;

    frame.displayPhase();

    std::cout
        << "Au joueur : "
        << frame.currentPlayer().getName()
        << std::endl;

    std::cout
        << frame.getPlayer1().getName()
        << " : "
        << frame.getPlayer1().getScore()
        << " | "
        << frame.getPlayer2().getName()
        << " : "
        << frame.getPlayer2().getScore()
        << std::endl;

    std::cout
        << "Rouges restantes : "
        << frame.redsRemaining()
        << std::endl;
}