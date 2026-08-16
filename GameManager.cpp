#include "GameManager.h"


GameManager::GameManager()
{

}



void GameManager::startMatch()
{
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
