#include "ShotHistory.h"

#include <iostream>


// =====================================
// Constructeur
// =====================================

ShotHistory::ShotHistory()
{

}


// =====================================
// Ajouter un coup
// =====================================

void ShotHistory::addShot(
    const Shot& shot
)
{
    m_shots.push_back(shot);

    LogEntry entry;
    entry.type = LogEntry::Type::Shot;
    entry.playerName = shot.getPlayerName();
    entry.ballName = shot.getBallName();
    entry.points = shot.getPoints();
    m_log.push_back(entry);
}


// =====================================
// Ajouter une faute
// =====================================

void ShotHistory::addFoul(
    const std::string& playerName,
    const std::string& requiredBall,
    const std::string& touchedBall,
    const std::string& reason,
    int points
)
{
    FoulRecord foul;

    foul.playerName = playerName;

    foul.requiredBall = requiredBall;

    foul.touchedBall = touchedBall;

    foul.reason = reason;

    foul.points = points;


    m_fouls.push_back(foul);

    LogEntry entry;
    entry.type = LogEntry::Type::Foul;
    entry.playerName = playerName;
    entry.requiredBall = requiredBall;
    entry.touchedBall = touchedBall;
    entry.reason = reason;
    entry.foulPoints = points;
    m_log.push_back(entry);
}


// =====================================
// Ajouter une fin de tour sans bille jouee
// =====================================

void ShotHistory::addMiss(const std::string& playerName)
{
    LogEntry entry;
    entry.type = LogEntry::Type::Miss;
    entry.playerName = playerName;
    m_log.push_back(entry);
}


// =====================================
// Ajouter un armement de "bille touchante"
// =====================================

void ShotHistory::addTouchingBall(const std::string& playerName, const std::string& ballName)
{
    LogEntry entry;
    entry.type = LogEntry::Type::TouchingBall;
    entry.playerName = playerName;
    entry.ballName = ballName;
    m_log.push_back(entry);
}


// =====================================
// Ajouter un "Faire rejouer"
// =====================================

void ShotHistory::addReplay(const std::string& playerName)
{
    LogEntry entry;
    entry.type = LogEntry::Type::Replay;
    entry.playerName = playerName;
    m_log.push_back(entry);
}


// =====================================
// Ajouter une correction d'arbitre
// =====================================

void ShotHistory::addCorrection(const std::string& before, const std::string& after)
{
    LogEntry entry;
    entry.type = LogEntry::Type::Correction;
    entry.playerName = "Arbitre";
    entry.correctionBefore = before;
    entry.correctionAfter = after;
    m_log.push_back(entry);
}


// =====================================
// Nombre de coups
// =====================================

int ShotHistory::getShotCount() const
{
    return static_cast<int>(m_shots.size());
}


// =====================================
// Journal chronologique unifie
// =====================================

const std::vector<LogEntry>& ShotHistory::getLog() const
{
    return m_log;
}


// =====================================
// Affichage historique
// =====================================

void ShotHistory::displayHistory() const
{
    std::cout
        << "=== HISTORIQUE DES COUPS ==="
        << std::endl;


    for (size_t i = 0; i < m_shots.size(); i++)
    {
        std::cout
            << "Coup "
            << i + 1
            << " : "
            << m_shots[i].getPlayerName()
            << " - "
            << m_shots[i].getBallName()
            << " - "
            << m_shots[i].getPoints()
            << " points"
            << std::endl;
    }


    if (!m_fouls.empty())
    {
        std::cout
            << std::endl
            << "=== FAUTES ==="
            << std::endl;


        for (const auto& foul : m_fouls)
        {
            std::cout
                << "FAUTE : "
                << foul.playerName
                << std::endl;


            std::cout
                << "Bille demandee : "
                << foul.requiredBall
                << std::endl;


            std::cout
                << "Bille jouee : "
                << foul.touchedBall
                << std::endl;


            std::cout
                << "Motif : "
                << foul.reason
                << std::endl;


            std::cout
                << "Penalite : "
                << foul.points
                << " points"
                << std::endl;
        }
    }
}