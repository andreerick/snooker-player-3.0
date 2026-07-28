#include <iostream>
#include <exception>

#include "GameManager.h"


int main()
{
    try
    {
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
    catch (const std::exception& exception)
    {
        std::cerr
            << "Erreur fatale: "
            << exception.what()
            << std::endl;
    }
    catch (...)
    {
        std::cerr
            << "Erreur fatale inconnue"
            << std::endl;
    }

    return 1;
}