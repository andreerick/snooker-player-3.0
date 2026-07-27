#include <iostream>

#include "GameManager.h"


int main()
{S
    std::cout << "=== TEST SNOOKER PLAYER ==="
        << std::endl;


    GameManager manager;


    std::cout << "=== Debut du match ==="
        << std::endl;


    manager.startMatch();


    std::cout << "Match lance"
        << std::endl;


    return 0;
}