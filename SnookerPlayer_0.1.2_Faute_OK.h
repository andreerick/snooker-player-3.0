#pragma once

#include "Player.h"
#include "Ball.h"
#include "ShotHistory.h"
#include "Referee.h"

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


    // Gestion des billes
    int redsRemaining() const;

    void potRed();

    void potColor(Ball ball);



    // Etat du jeu
    bool isColorNeeded() const;

    bool isFinished() const;

    FramePhase getPhase() const;



    // Jeu normal
    bool playShot(const Ball& ball);

    bool playTurn(const Ball& ball);



    // Arbitrage
    bool checkShot(const Ball& intended, const Ball& hit);

    void foul(int points);

    void missShot();



    // Couleurs finales
    bool isCorrectFinalColor(const Ball& ball) const;

    std::string getNextColorName() const;

    Ball getRequiredBall() const;



    // Affichage
    void displayPhase() const;

    void displayStatus() const;



    // Historique
    const ShotHistory& getHistory() const;



private:

    Player m_player1;

    Player m_player2;


    Player* m_currentPlayer = nullptr;

    Referee m_referee;


    int m_redsRemaining;


    bool m_needColor;


    int m_nextColor;


    FramePhase m_phase;


    ShotHistory m_history;

};