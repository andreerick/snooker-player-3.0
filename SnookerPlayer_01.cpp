#include <iostream>

#include "GameManager.h"
#include "Frame.h"
#include "Ball.h"


int main()
{
    std::cout << "=== TEST SNOOKER PLAYER ===" << std::endl;

    GameManager manager;

    // Nommer les joueurs
    manager.getMatch().getPlayer1().setName("Joueur 1");
    manager.getMatch().getPlayer2().setName("Joueur 2");

    std::cout << "=== Debut du match ===" << std::endl;

    manager.startMatch();

    std::cout << "Match lance" << std::endl;

    // --- Demonstration de la logique snooker ---
    Frame& frame = manager.getMatch().getCurrentFrame();

    std::cout << "\n--- Phase initiale ---" << std::endl;
    frame.displayPhase();
    std::cout << "Rouges restantes : " << frame.redsRemaining() << std::endl;

    // Coup 1 : Rouge
    std::cout << "\n--- Coup 1 : Rouge ---" << std::endl;
    frame.playShot(Ball("Rouge", 1));
    std::cout << "Score " << frame.getPlayer1().getName()
              << " : " << frame.getPlayer1().getScore() << std::endl;

    // Coup 2 : Noir (couleur apres rouge)
    std::cout << "\n--- Coup 2 : Noir (couleur apres rouge) ---" << std::endl;
    frame.playShot(Ball("Noir", 7));
    std::cout << "Score " << frame.getPlayer1().getName()
              << " : " << frame.getPlayer1().getScore() << std::endl;

    // Coup 3 : Rouge (tour suivant du meme joueur)
    std::cout << "\n--- Coup 3 : Rouge ---" << std::endl;
    frame.playShot(Ball("Rouge", 1));

    // Coup 4 : Faute - mauvaise bille (devrait etre couleur)
    std::cout << "\n--- Coup 4 : Faute (Rouge au lieu d'une couleur) ---" << std::endl;
    frame.playShot(Ball("Rouge", 1));
    std::cout << "Score " << frame.getPlayer2().getName()
              << " (penalite) : " << frame.getPlayer2().getScore() << std::endl;

    // Affichage de l'historique
    std::cout << std::endl;
    frame.getHistory().displayHistory();

    return 0;
}