#pragma once

#include <vector>
#include "Player.h"



class Tournament
{

public:

    Tournament();


    void addPlayer(const Player& player);


    int getPlayerCount() const;


    const std::vector<Player>& getPlayers() const;



private:

    std::vector<Player> m_players;

};
