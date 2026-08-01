#pragma once
#include <string>

// Represente une faute enregistree dans l'historique de la frame.
// Ce fichier etait référencé par ShotHistory.h mais absent du dépôt.
struct FoulRecord
{
    std::string playerName;
    std::string requiredBall;
    std::string touchedBall;
    std::string reason;
    int points = 0;
};
