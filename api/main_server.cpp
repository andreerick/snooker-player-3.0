#include <iostream>
#include <csignal>
#include <stdexcept>

#include "../GameManager.h"
#include "../camera/CameraConfig.h"
#include "../camera/CameraFusion.h"
#include "ApiServer.h"

// Pointeur global pour le gestionnaire de signal
static Api::ApiServer* g_server = nullptr;

void signalHandler(int sig)
{
    if (g_server)
    {
        std::cout << "\nArret du serveur (signal " << sig << ")..." << std::endl;
        g_server->stop();
    }
}

int main(int argc, char* argv[])
{
    int port = 8080;
    if (argc > 1)
    {
        try { port = std::stoi(argv[1]); }
        catch (...) { /* port par defaut */ }
    }

    std::cout << "=== Snooker Player 3.0 - Serveur API ===" << std::endl;

    // Initialiser le moteur de jeu
    GameManager manager;
    manager.getMatch().getPlayer1().setName("Joueur 1");
    manager.getMatch().getPlayer2().setName("Joueur 2");
    manager.startMatch();

    // Initialiser le systeme de cameras (3 cameras plongeantes)
    auto camConfigs = Camera::TripleCameraSystem::getDefaultConfig();
    Camera::CameraFusion fusion(camConfigs);

    std::cout << "Configuration cameras :" << std::endl;
    for (const auto& cam : camConfigs)
    {
        std::cout << "  Camera " << cam.id
                  << " : position=" << cam.position_mm << "mm"
                  << ", zone=[" << cam.zone_start_mm << "-" << cam.zone_end_mm << "]mm"
                  << ", hauteur=" << cam.height_mm << "mm"
                  << std::endl;
    }

    // Demarrer le serveur
    try
    {
        Api::ApiServer server(manager, fusion, port);
        g_server = &server;

        std::signal(SIGINT,  signalHandler);
        std::signal(SIGTERM, signalHandler);

        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
