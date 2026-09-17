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
    // TouchingBall = armement de la "bille touchante" (voir Frame::
    // setTouchingBall()) : meme raison d'etre que Miss, sans quoi ce
    // geste d'arbitrage resterait invisible et non rejouable. Le nom de
    // la bille annoncee (purement informatif) est stocke dans le champ
    // `ballName` ci-dessous, partage avec Shot.
    // Replay = "Faire rejouer" (reglement Sect. 3 §13) : le non-fautif
    // choisit de faire rejouer le fautif plutot que de prendre la
    // position telle quelle (voir Frame::requestReplay()). Sans cette
    // entree, le journal ne distingue pas ce choix de "prendre la
    // table", et un scenario rejoue attribuerait les coups suivants au
    // mauvais joueur (le rejeu ignore les noms de joueurs du fichier et
    // suit uniquement l'etat reel du moteur).
    // Correction = l'arbitre corrige le DERNIER coup enregistre (ex. la
    // camera avait detecte la mauvaise bille) : voir Frame::logCorrection(),
    // qui suit un "Retour" (annulation) puis le vrai coup rejoue. Garde une
    // trace explicite ("avant -> apres") au lieu de laisser l'erreur
    // disparaitre silencieusement du journal.
    enum class Type { Shot, Foul, Miss, TouchingBall, Replay, Correction };

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

    // Rempli si type == Correction (voir Frame::logCorrection()) : description
    // courte du coup erronement enregistre puis de celui qui le remplace.
    std::string correctionBefore;
    std::string correctionAfter;
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

    // Ajouter un armement de "bille touchante" (voir Frame::setTouchingBall()).
    // `ballName` est optionnel (annonce non renseignee).
    void addTouchingBall(const std::string& playerName, const std::string& ballName = "");

    // Ajouter un "Faire rejouer" (voir Frame::requestReplay()). playerName
    // est celui qui REJOUE (le fautif, apres le changement de joueur),
    // pas celui qui a fait ce choix.
    void addReplay(const std::string& playerName);

    // Ajouter une correction d'arbitre (voir Frame::logCorrection()).
    void addCorrection(const std::string& before, const std::string& after);


    int getShotCount() const;


    void displayHistory() const;


    // Journal chronologique unifie (coups + fautes), utilise par l'UI
    const std::vector<LogEntry>& getLog() const;



private:

    std::vector<Shot> m_shots;

    std::vector<FoulRecord> m_fouls;

    std::vector<LogEntry> m_log;

};