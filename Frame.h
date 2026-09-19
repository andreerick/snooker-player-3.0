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

    // Concession de frame (Sect. 3 du reglement) : un joueur abandonne la
    // frame en cours sur decision de l'arbitre -- l'ADVERSAIRE gagne
    // immediatement, quel que soit le score actuel (contrairement a
    // forceFinishFrame() qui se contente de regarder qui est devant).
    // Renvoie false (et ne fait rien) si la frame est deja terminee.
    bool concedeFrame(Player& conceder);



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


    // Touching Ball (Bille en contact, voir Sect. 3 §8 du reglement) :
    // arme par l'arbitre juste avant un coup ou la blanche est deja au
    // repos en contact avec une bille jouable. Dans ce cas le premier
    // contact requis est considere comme deja rempli (§8(c)(i)) : le
    // prochain coup ne peut plus etre sanctionne pour "mauvaise bille en
    // premier contact", quelle que soit la bille reellement touchee ou
    // empochee ensuite (voir playShot()). Se desarme automatiquement
    // apres ce seul coup, comme le Free Ball. Ne couvre PAS le poussé de
    // bille si la bille touchante bouge anormalement : ca reste a
    // l'appreciation de l'arbitre humain via le bouton "Faute" classique.
    // `ballName` est purement informatif (trace dans le journal, voir
    // ShotHistory::addTouchingBall()) : la bille annoncee ne change rien
    // au contournement lui-meme, qui s'applique au prochain coup quel
    // qu'il soit.
    void setTouchingBall(bool value, const std::string& ballName = "");

    bool isTouchingBall() const;


    // "Faire rejouer" (reglement Sect. 3 §13) : le non-fautif choisit de
    // faire rejouer le fautif (bouton "Remettre en place", present a la
    // fois dans le panneau de choix apres un Miss et dans le menu du
    // bouton "Free ball") plutot que de prendre la position telle quelle.
    // Remplace un appel direct a switchPlayer() pour que ce choix soit
    // enregistre dans le journal (voir ShotHistory::addReplay()) --
    // indispensable pour qu'un scenario rejoue attribue les coups
    // suivants au bon joueur.
    void requestReplay();

    // Motif enregistre pour une faute "Absence de veritable tentative
    // (Miss)" (voir Frame::missReplayWarningPlayer()).
    static constexpr const char* kMissFoulReason = "Absence de veritable tentative (Miss)";

    // Sect. 3 §14(d) : nombre de "Faute et Miss" CONSECUTIVES suivies
    // chacune d'un "Faire rejouer" (le meme joueur rejoue depuis la
    // position d'origine) a la fin du journal. 0 si le journal ne se
    // termine pas par un "Faire rejouer" de ce type -- la chaine est
    // rompue par n'importe quel autre evenement (coup reussi, Fin de
    // break, adversaire qui prend la table...). `player` recoit le nom du
    // joueur qui rejoue (a avertir).
    int missReplayChain(std::string& player) const;

    // Vrai si le journal se termine par une 3e "Faute et Miss" de suite
    // (deja 2 rejouees avant) : la frame peut etre attribuee a
    // l'adversaire. `offender` recoit le nom du fautif. Sans effet sur la
    // frame -- c'est l'appelant qui demande confirmation puis appelle
    // concedeFrame().
    bool isMissFrameForfeitDue(std::string& offender) const;

    // Correction d'arbitre (voir MainWindow, bouton "Correction arbitre") :
    // enregistre une trace explicite "avant -> apres" dans le journal.
    // N'agit PAS sur le score/l'etat du jeu -- l'appelant a deja annule le
    // mauvais coup (via le meme mecanisme que "Retour") et rejoue le bon
    // avant d'appeler ceci, pour que ce soit le VRAI coup (playShot/foul)
    // qui recalcule le score, pas une simple correction de chiffre.
    void logCorrection(const std::string& before, const std::string& after);


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

    // Sect. 2 §1(b)/(c) du reglement : des que la noire devient la SEULE
    // bille objet restant sur la table et que l'ecart de points depasse
    // deja 7 (valeur max d'une seule bille), la frame est gagnee sur-le-
    // champ -- meme si la noire n'a pas encore ete jouee. Pur calcul (pas
    // de jugement d'arbitre a faire, contrairement a "bille touchante" ou
    // au Pat), donc verifie automatiquement a chaque endroit ou la phase
    // peut entrer dans cet etat ou le score peut changer pendant celui-ci
    // (potColor(), foul(), playFreeBall()). Ne fait rien si la frame est
    // deja terminee ou si la noire n'est pas (encore) la seule bille
    // restante.
    void checkInsurmountableLead();

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

    // Non nul si la frame s'est terminee par une concession (voir
    // concedeFrame()) : getWinnerName() doit alors declarer l'AUTRE joueur
    // vainqueur sans regarder le score, qui peut tres bien favoriser le
    // joueur qui concede (concession volontaire, pas une simple avance).
    Player* m_concededBy = nullptr;

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

    // Voir setTouchingBall()/isTouchingBall().
    bool m_touchingBall = false;

    int m_nextColor;


    FramePhase m_phase;


    ShotHistory m_history;

    BallSet m_ballSet;

};