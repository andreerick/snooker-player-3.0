#pragma once

#include "../GameManager.h"
#include "../camera/CameraFusion.h"

#include <string>
#include <functional>
#include <map>

namespace Api {

// Requete HTTP minimale
struct HttpRequest
{
    std::string method;  // GET, POST, ...
    std::string path;
    std::string body;
    std::map<std::string, std::string> pathParams; // parametres d'URL {id}
};

// Reponse HTTP
struct HttpResponse
{
    int         status  = 200;
    std::string content_type = "application/json";
    std::string body;
};

// Gestionnaire de route
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

// Serveur REST minimal (TCP/HTTP)
//
// Endpoints:
//   GET  /health                    - Sante du serveur
//   GET  /api/game/state            - Etat courant du jeu
//   POST /api/game/shot             - Jouer un coup  {"ball_name":"Noir","ball_value":7}
//   POST /api/game/foul             - Declarer une faute {"points":4}
//   GET  /api/game/history          - Historique des coups
//   POST /api/camera/{id}/frame     - Soumettre des detections de billes
//   GET  /api/camera/fused          - Etat fusionne des cameras
//   GET  /api/simulate/demo         - Simulation de quelques coups
class ApiServer
{
public:
    ApiServer(GameManager& manager,
              Camera::CameraFusion& fusion,
              int port = 8080);

    // Demarre le serveur (bloquant)
    void run();

    // Demarre le serveur (non-bloquant) - retourne quand pret
    void start();

    // Arrete le serveur
    void stop();

    int getPort() const { return m_port; }

private:
    // Enregistre les routes
    void registerRoutes();

    // Traite une requete HTTP brute et retourne la reponse
    HttpResponse handleRequest(const HttpRequest& req);

    // Handlers des routes
    HttpResponse handleHealth(const HttpRequest& req);
    HttpResponse handleGameState(const HttpRequest& req);
    HttpResponse handleGameShot(const HttpRequest& req);
    HttpResponse handleGameFoul(const HttpRequest& req);
    HttpResponse handleGameHistory(const HttpRequest& req);
    HttpResponse handleCameraFrame(const HttpRequest& req);
    HttpResponse handleCameraFused(const HttpRequest& req);
    HttpResponse handleSimulateDemo(const HttpRequest& req);

    // Parse une requete HTTP brute (socket)
    HttpRequest  parseRequest(const std::string& raw);

    // Serialise une reponse HTTP pour envoi socket
    std::string  serializeResponse(const HttpResponse& resp);

    GameManager&           m_manager;
    Camera::CameraFusion&  m_fusion;
    int                    m_port;
    bool                   m_running = false;
    int                    m_serverFd = -1;

    // Table de routage : (method, path_prefix) -> handler
    struct Route
    {
        std::string method;
        std::string path;
        RouteHandler handler;
    };
    std::vector<Route> m_routes;
};

} // namespace Api
