#pragma once

#include "Player.h"
#include "Frame.h"
#include <string>
#include <vector>

// Resultat d'une frame terminee (vainqueur + score final des deux
// joueurs), conserve par le Match une fois la frame remplacee par une
// frame fraiche, pour garder l'historique de la frame en cours de match
// (utilise notamment pour la sauvegarde du match).
struct FrameResult
{
    std::string winnerName;
    int scorePlayer1 = 0;
    int scorePlayer2 = 0;
};

class Match
{

public:

    Match();

    void start();

    Player& getPlayer1();

    Player& getPlayer2();


    Frame& getCurrentFrame();


    void startNewFrame();

    void setPlayerNames(const std::string& name1, const std::string& name2);

    // A appeler juste apres la construction (avant start()) : nombre de
    // frames a gagner pour remporter le match (2 = meilleur des 3, 3 =
    // meilleur des 5, etc.). Ignore les valeurs non positives.
    void setFramesToWin(int frames);

    void checkFrameEnd();


    void frameWon(Player& player);


    int getFramesPlayer1() const;

    int getFramesPlayer2() const;


    bool isMatchFinished() const;

    int getFramesToWin() const;

    // Indique que la frame en cours vient de se terminer et qu'une
    // nouvelle frame attend d'etre demarree (le score final reste
    // affiche tant que proceedToNextFrame() n'a pas ete appele).
    bool isFrameJustFinished() const;

    // A appeler explicitement (apres un delai d'affichage cote UI, par
    // exemple) pour reellement demarrer la frame suivante.
    void proceedToNextFrame();

    // Resultat de chaque frame jouee jusqu'ici dans le match (vainqueur
    // et score final), dans l'ordre chronologique.
    const std::vector<FrameResult>& getFrameResults() const;

    // A appeler quand le coup qui vient d'etre annule (bouton "Retour")
    // etait celui qui venait de terminer la frame en cours : sans cela,
    // restaurer directement Frame (getCurrentFrame() = ancienSnapshot)
    // ne touche que l'objet Frame, laissant m_awaitingNextFrame et le
    // tally de frames gagnees (m_framesPlayer1/2) bloques sur un
    // resultat qui n'est plus valide. Ne fait rien si aucune frame
    // n'est actuellement en attente de la frame suivante.
    void undoFrameConclusion();

    // "Conceder le match" (abandon definitif, ex: joueur parti sans
    // revenir) : force le tally de frames du VAINQUEUR directement a
    // m_framesToWin, quel que soit le score actuel, pour que
    // isMatchFinished() devienne vrai immediatement. A appeler APRES avoir
    // conceder la frame en cours (Frame::concedeFrame() + afterShot()) --
    // ne s'occupe que du tally global, pas de la frame en cours.
    void forceMatchEnd(Player& winner);



private:

    Player m_player1;

    Player m_player2;


    Frame m_currentFrame;


    int m_framesPlayer1;

    int m_framesPlayer2;


    int m_framesToWin;

    // Vrai juste apres la fin d'une frame, tant que proceedToNextFrame()
    // n'a pas encore ete appele pour reellement demarrer la suivante.
    bool m_awaitingNextFrame = false;

    std::vector<FrameResult> m_frameResults;

};