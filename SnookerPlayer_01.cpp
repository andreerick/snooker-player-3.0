#include <iostream>

#include "GameManager.h"
#include "Ball.h"
#include "BallSet.h"



int main()
{

    std::cout
        << "=== TEST SNOOKER PLAYER 0.1 ==="
        << std::endl;


    GameManager manager;


    std::cout
        << std::endl
        << "=== Debut du match ==="
        << std::endl;


    manager.startMatch();


    Frame& frame =
        manager.getMatch()
        .getCurrentFrame();



    frame.displayStatus();



    // ==================================
    // TEST 1
    // Coup normal
    // ==================================

    Ball rouge("Rouge", 1);

    Ball noire("Noire", 7);



    std::cout
        << std::endl
        << "=== TEST 1 : COUP NORMAL ==="
        << std::endl;


    std::cout
        << "Eric joue une rouge"
        << std::endl;


    frame.playShot(rouge);



    std::cout
        << "Eric joue la noire"
        << std::endl;


    frame.playShot(noire);



    frame.displayStatus();





    // ==================================
    // TEST 2
    // Faute
    // ==================================

    std::cout
        << std::endl
        << "=== TEST 2 : FAUTE ==="
        << std::endl;



    frame.switchPlayer();



    std::cout
        << "Jean joue une noire alors qu'une rouge est demandee"
        << std::endl;


    frame.playShot(noire);



    frame.displayStatus();





    // ==================================
    // TEST 3
    // Reprise joueur suivant
    // ==================================

    std::cout
        << std::endl
        << "=== TEST 3 : REPRISE APRES FAUTE ==="
        << std::endl;



    std::cout
        << "Joueur actuel : "
        << frame.currentPlayer().getName()
        << std::endl;





    // ==================================
    // Historique
    // ==================================

    std::cout
        << std::endl
        << "=== HISTORIQUE ==="
        << std::endl;



    frame.getHistory()
        .displayHistory();





    // ==================================
    // TEST FREE BALL
    // ==================================

    std::cout
        << std::endl
        << "=== TEST FREE BALL ==="
        << std::endl;



    frame.setFreeBall(true);



    if (frame.isFreeBall())
    {
        std::cout
            << "FREE BALL ACTIVE"
            << std::endl;
    }
    else
    {
        std::cout
            << "FREE BALL INACTIVE"
            << std::endl;
    }



    Ball bleue("Bleue", 5);



    frame.setFreeBallColor(bleue);

    std::cout
        << "Joueur joue la Bleue en Free Ball"
        << std::endl;


    frame.playFreeBall(bleue);

    std::cout
        << "Couleur Free Ball : "
        << frame.getFreeBallColor().getName()
        << std::endl;


    
    frame.setFreeBall(false);


    // ==================================
// TEST BALL SET
// ==================================

    std::cout
        << std::endl
        << "=== TEST BALLE SET ==="
        << std::endl;


    BallSet table;


    std::cout
        << "Bleue presente : "
        << (table.isOnTable("Bleue") ? "OUI" : "NON")
        << std::endl;



    std::cout
        << "Retrait Bleue"
        << std::endl;


    table.removeBall("Bleue");



    std::cout
        << "Bleue presente : "
        << (table.isOnTable("Bleue") ? "OUI" : "NON")
        << std::endl;



    std::cout
        << "Remise Bleue"
        << std::endl;


    table.restoreBall(
        Ball("Bleue", 5)
    );


    std::cout
        << "Bleue presente : "
        << (table.isOnTable("Bleue") ? "OUI" : "NON")
        << std::endl;

    // ==================================
    // TEST COMPTE ROUGES BALLSET
    // ==================================

    std::cout
        << std::endl
        << "=== TEST ROUGES BALLSET ==="
        << std::endl;


    BallSet tableRouges;


    std::cout
        << "Rouges au depart : "
        << tableRouges.countBalls("Rouge")
        << std::endl;


    tableRouges.removeBall("Rouge");


    std::cout
        << "Rouges apres retrait : "
        << tableRouges.countBalls("Rouge")
        << std::endl;

    std::cout
        << std::endl

        << "=== FIN TEST ==="
        << std::endl;


    // ==================================
    // TEST FRAME + BALLSET
    // ==================================

    std::cout
        << std::endl
        << "=== TEST FRAME + BALLSET ==="
        << std::endl;


    GameManager testManager;

    testManager.startMatch();


    Frame& testFrame =
        testManager.getMatch()
        .getCurrentFrame();



    Ball rougeTest("Rouge", 1);

    Ball bleueTest("Bleue", 5);



    std::cout
        << "Joueur joue Rouge"
        << std::endl;


    testFrame.playShot(rougeTest);



    std::cout
        << "Bleue avant coup couleur : "
        << (testFrame.getBallSet().isOnTable("Bleue") ? "OUI" : "NON")
        << std::endl;



    std::cout
        << "Joueur joue Bleue"
        << std::endl;


    testFrame.playShot(bleueTest);



    std::cout
        << "Bleue apres coup couleur : "
        << (testFrame.getBallSet().isOnTable("Bleue") ? "OUI" : "NON")
        << std::endl;
    return 0;
}