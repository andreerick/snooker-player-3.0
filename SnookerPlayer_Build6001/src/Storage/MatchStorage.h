#pragma once

#include <QJsonArray>
#include <QString>

class Match;

// Sauvegarde le resultat d'un match (joueurs, score de frames, detail
// par frame) dans un fichier JSON local (matchs.json, a cote de
// l'executable), pour garder un historique des matchs joues.
class MatchStorage
{
public:
    // `historyIndex` permet de mettre a jour la MEME entree au fil du
    // match plutot que d'en ajouter une nouvelle a chaque appel : -1 la
    // premiere fois (une nouvelle entree est alors ajoutee et son index
    // ecrit dans `historyIndex` pour les appels suivants), sinon l'entree
    // a cet index est REMPLACEE en place. Appele a la fois apres chaque
    // frame terminee et a la fin du match (voir MainWindow::refreshDisplay()),
    // pour qu'un plantage en cours de match ne fasse pas disparaitre les
    // frames deja jouees de l'historique/Statistiques.
    static void saveMatch(Match& match, int& historyIndex);

    // Lit l'historique complet des matchs sauvegardes (tableau vide si le
    // fichier n'existe pas encore).
    static QJsonArray loadHistory();

private:
    static QString filePath();
};