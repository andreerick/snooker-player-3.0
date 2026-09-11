#include "GameManager.h"


GameManager::GameManager()
{

}



void GameManager::startMatch()
{
    m_match.start();
}



void GameManager::startNewMatch(const std::string& name1, const std::string& name2, int framesToWin)
{
    m_match = Match();
    m_match.getPlayer1().setName(name1);
    m_match.getPlayer2().setName(name2);
    m_match.setFramesToWin(framesToWin);
    m_match.start();
}



Match& GameManager::getMatch()
{
    return m_match;
}

void GameManager::afterShot()
{
    m_match.checkFrameEnd();
}