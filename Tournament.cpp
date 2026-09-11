#include "Tournament.h"



Tournament::Tournament()
{

}



void Tournament::addPlayer(const Player& player)
{
    m_players.push_back(player);
}




int Tournament::getPlayerCount() const
{
    return static_cast<int>(m_players.size());
}




const std::vector<Player>& Tournament::getPlayers() const
{
    return m_players;
}