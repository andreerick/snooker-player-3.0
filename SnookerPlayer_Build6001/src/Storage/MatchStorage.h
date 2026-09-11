#pragma once

#include <QJsonArray>
#include <QString>

class Match;

// Sauvegarde le resultat d'un match termine (joueurs, score de frames,
// detail par frame) dans un fichier JSON local (matchs.json, a cote de
// l'executable), pour garder un historique des matchs joues.
class MatchStorage
{
public:
    static void saveMatch(Match& match);

    // Lit l'historique complet des matchs sauvegardes (tableau vide si le
    // fichier n'existe pas encore).
    static QJsonArray loadHistory();

private:
    static QString filePath();
};