#pragma once

#include "Match.h"


class GameManager
{

public:

    GameManager();


    void startMatch();

    // Remplace le match en cours par un match tout neuf (scores et
    // frames a zero) avec les noms de joueurs donnes, et le demarre.
    // Utilise pour "Nouveau match" sans avoir a relancer l'application.
    // framesToWin : 2 = meilleur des 3 frames (valeur par defaut), 3 =
    // meilleur des 5, etc. (voir Match::setFramesToWin()).
    void startNewMatch(const std::string& name1, const std::string& name2, int framesToWin = 2);


    Match& getMatch();

    void afterShot();


private:

    Match m_match;

};