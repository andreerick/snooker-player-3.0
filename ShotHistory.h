#pragma once

#include <vector>
#include "Shot.h"


class ShotHistory
{

public:

    ShotHistory();


    void addShot(const Shot& shot);


    int getShotCount() const;


    void displayHistory() const;


private:

    std::vector<Shot> m_shots;

};