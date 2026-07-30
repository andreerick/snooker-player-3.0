#pragma once

#include <vector>
#include <string>

#include "Shot.h"
#include "FoulRecord.h"



class ShotHistory
{

public:

    ShotHistory();


    // Ajouter un coup réussi
    void addShot(
        const Shot& shot
    );


    // Ajouter une faute
    void addFoul(
        const std::string& playerName,
        const std::string& requiredBall,
        const std::string& touchedBall,
        const std::string& reason,
        int points
    );


    int getShotCount() const;


    void displayHistory() const;



private:

    std::vector<Shot> m_shots;

    std::vector<FoulRecord> m_fouls;

};