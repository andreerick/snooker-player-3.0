#pragma once

#include <memory>
#include <thread>

namespace httplib { class Server; }

// =====================================================================
// CueSenseLauncher
// ---------------------------------------------------------------------
// Lance CueSense (projet web separe, voir C:\CueSense -- capteur de
// mouvement pour l'entrainement, https://github.com/YohannParis/cuesense)
// dans le navigateur par defaut, depuis l'ecran d'accueil de Snooker
// Player ("Entrainement").
//
// CueSense est un site statique (HTML/CSS/JS purs, pas de backend) qui
// fonctionne meme ouvert directement en file://, MAIS le Web Bluetooth
// (necessaire pour lire le capteur en direct) exige un contexte securise
// (voir son README) : file:// n'est pas garanti d'etre traite comme tel
// par tous les navigateurs. On sert donc les fichiers via un petit
// serveur HTTP local (cpp-httplib, deja vendorise pour MatchWebServer)
// plutot que d'ouvrir index.html directement.
// =====================================================================
class CueSenseLauncher
{
public:
    CueSenseLauncher();
    ~CueSenseLauncher();

    // Demarre le serveur de fichiers statiques si besoin (une seule fois,
    // les appels suivants reutilisent le meme serveur), puis ouvre
    // CueSense dans le navigateur par defaut. Renvoie false si le
    // dossier CueSense est introuvable ou si le serveur n'a pas pu
    // demarrer (aucun port disponible).
    bool launch();

private:
    std::unique_ptr<httplib::Server> m_server;
    std::thread m_serverThread;
    int m_port = 0;
    bool m_started = false;
};
