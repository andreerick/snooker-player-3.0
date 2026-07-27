#include <iostream>

#include "GameManager.h"


int main()
{
    std::cout << "=== TEST GAME MANAGER ==="
        << std::endl;


    GameManager manager;


    manager.startMatch();


    std::cout << "Match lance"
        << std::endl;


    return 0;
}