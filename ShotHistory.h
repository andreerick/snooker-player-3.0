#pragma once

#include <vector>
#include <string>

#include "Shot.h"
#include "FoulRecord.h"


// Entree unifiee du journal, dans l'ordre chronologique reel
// (contrairement aux vecteurs m_shots / m_fouls qui sont separes).
// Permet a l'UI (MoveLogWidget) d'afficher coups et fautes melanges
// dans l'ordre ou ils se sont produits.
struct LogEntry
{
    // Miss = fin de tour sans bille jouee ("Fin de break") : n'existait
    // pas dans le journal avant (silencieux), ajoute pour que le journal
    // soit un enregistrement fidele et rejouable de la partie.
    enum class Type { Shot, Foul, Miss };

    Type type = Type::Shot;

    std::string playerName;

    // Rempli si type == Shot
    std::string ballName;
    int points = 0;

    // Rempli si type == Foul
    std::string requiredBall;
    std::string touchedBall;
    std::string reason;
    int foulPoints = 0;
};


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

    // Ajouter une fin de tour sans bille jouee ("Fin de break").
    void addMiss(const std::string& playerName);


    int getShotCount() const;


    void displayHistory() const;


    // Journal chronologique unifie (coups + fautes), utilise par l'UI
    const std::vector<LogEntry>& getLog() const;



private:

    std::vector<Shot> m_shots;

    std::vector<FoulRecord> m_fouls;

    std::vector<LogEntry> m_log;

};