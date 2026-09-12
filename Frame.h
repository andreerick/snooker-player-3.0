#pragma once

#include "Player.h"
#include "Ball.h"
#include "ShotHistory.h"
#include "Referee.h"
#include "BallSet.h"
#include <string>


enum class FramePhase
{
    Reds,
    LastRedColor,
    FinalColors,
    // Egalite au moment ou la noire est empochee : elle est respotee et
    // rejouee (regle officielle du snooker) jusqu'a ce qu'un joueur
    // prenne l'avantage.
    BlackReplay,
    Finished
};


class Frame
{

public:

    Frame();

    Frame(const Frame& other);
    Frame& operator=(const Frame& other);

    // Joueurs
    Player& currentPlayer();

    Player& getPlayer1();

    Player& getPlayer2();


    void switchPlayer();

    void setPlayerNames(const std::string& name1, const std::string& name2);


    // Gestion des billes
    int redsRemaining() const;

    void potRed();

    void potColor(Ball ball);

    int pointsRemaining() const;



    // Etat du jeu
    bool isColorNeeded() const;

    bool isFinished() const;

    FramePhase getPhase() const;

    std::string getWinnerName() const;

    // Force la fin de la frame en cours (declare simplement la phase
    // terminee) : le vainqueur est determine avec le score actuel.
    // Utilise par la telecommande de test pour sauter directement a
    // la fin de la frame sans jouer toutes les billes restantes.
    // Renvoie false (et ne fait rien) si les scores sont a egalite : un
    // arbitre ne peut pas non plus declarer une frame terminee sur une
    // egalite stricte (il faudrait rejouer la noire respotee, voir
    // FramePhase::BlackReplay) ; sans ce refus, Match::checkFrameEnd()
    // reste bloque (getWinnerName() renvoie "Egalite", cas qu'il ignore).
    bool forceFinishFrame();



    // Jeu normal
    bool playShot(const Ball& ball);

    bool playTurn(const Ball& ball);


    // Gestion Free Ball

    bool playFreeBall(const Ball& ball);



    // Arbitrage
    // `reason` est le motif affiche dans le journal des coups (ex: "Mauvaise
    // bille touchee" par defaut, ou "Bille sortie de la table" pour ce cas
    // precis) : le calcul de la penalite (min 4, max entre bille demandee
    // et bille concernee) est identique quel que soit le motif reel.
    void foul(
        const Ball& required,
        const Ball& touched,
        int points,
        const std::string& reason = "Mauvaise bille touchee"
    );

    void missShot();

    // Free Ball

    void setFreeBall(bool value);

    bool isFreeBall() const;

    void setFreeBallColor(const Ball& ball);

    Ball getFreeBallColor() const;

    // Vrai si, au moment de l'armement du Free Ball, la bille normalement
    // due etait ambigue ("n'importe quelle couleur", voir getRequiredBall())
    // -- dans ce cas la valeur a compter doit etre annoncee explicitement
    // via setFreeBallValue() avant de pouvoir jouer le coup (voir
    // MainWindow::handleBallAction(), PendingAction::ArmFreeBallValue).
    bool isFreeBallValueAmbiguous() const;

    // Annonce explicitement la valeur que le Free Ball doit compter (cas
    // ambigu seulement, voir isFreeBallValueAmbiguous()).
    void setFreeBallValue(const Ball& ball);


    // Couleurs finales
    bool isCorrectFinalColor(const Ball& ball) const;

    std::string getNextColorName() const;

    Ball getRequiredBall() const;



    // Affichage
    void displayPhase() const;

    void displayStatus() const;



    // Historique
    const ShotHistory& getHistory() const;

    // Table de jeu

    const BallSet& getBallSet() const;


private:

    Player m_player1;

    Player m_player2;


    Player* m_currentPlayer = nullptr;

    Referee m_referee;

    // Joueur qui a empoche la derniere rouge (voir playShot, Cas 1) :
    // permet au MEME joueur d'empocher plusieurs rouges d'affilee sans
    // couleur intercalee (frequent sur la casse, un seul coup reel),
    // tout en continuant a sanctionner un AUTRE joueur qui tenterait une
    // rouge alors qu'une couleur est due (cas reellement fautif).
    Player* m_lastRedPotter = nullptr;

    int m_redsRemaining;


    bool m_needColor;

    bool m_freeBall;

    Ball m_freeBallColor = Ball("Aucune", 0);

    // Ce que le Free Ball remplace, capture au moment ou il est arme (voir
    // setFreeBall()) : vrai si une ROUGE etait due (une couleur devient
    // due ensuite, comme pour une vraie rouge), faux si une COULEUR etait
    // due (une rouge redevient due ensuite, et la sequence des couleurs
    // finales avance si applicable). Necessaire pour que playFreeBall()
    // fasse progresser la partie correctement, comme un coup normal.
    bool m_freeBallForRed = true;

    // Valeur que le Free Ball doit compter s'il est reussi (voir
    // playFreeBall()) : deduite automatiquement de Frame::getRequiredBall()
    // au moment de l'armement (rouge = 1, couleur des couleurs finales =
    // sa valeur reelle), OU annoncee explicitement via setFreeBallValue()
    // quand getRequiredBall() est ambigu ("n'importe quelle couleur").
    Ball m_freeBallValueBall = Ball("Rouge", 1);

    int m_nextColor;


    FramePhase m_phase;


    ShotHistory m_history;

    BallSet m_ballSet;

};