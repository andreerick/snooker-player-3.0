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



    // Etat du jeu
    bool isColorNeeded() const;

    bool isFinished() const;

    FramePhase getPhase() const;

    std::string getWinnerName() const;



    // Jeu normal
    bool playShot(const Ball& ball);

    bool playTurn(const Ball& ball);


    // Gestion Free Ball

    bool playFreeBall(const Ball& ball);



    // Arbitrage
    bool checkShot(const Ball& intended, const Ball& hit);

    void foul(
        const Ball& required,
        const Ball& touched,
        int points
    );

    void missShot();

    // Free Ball

    void setFreeBall(bool value);

    bool isFreeBall() const;

    void setFreeBallColor(const Ball& ball);

    Ball getFreeBallColor() const;


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


    int m_redsRemaining;


    bool m_needColor;

    bool m_freeBall;

    Ball m_freeBallColor = Ball("Aucune", 0);

    int m_nextColor;


    FramePhase m_phase;


    ShotHistory m_history;

    BallSet m_ballSet;

};
