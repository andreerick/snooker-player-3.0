#include "ApiServer.h"
#include "JsonSerializer.h"
#include "../Frame.h"
#include "../Shot.h"
#include "../ShotHistory.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <chrono>

// Includes POSIX pour le serveur TCP
#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   using ssize_t = int;
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#endif

namespace Api {

// -------------------------------------------------------------------
// Construction / initialisation
// -------------------------------------------------------------------

ApiServer::ApiServer(GameManager& manager,
                     Camera::CameraFusion& fusion,
                     int port)
    : m_manager(manager)
    , m_fusion(fusion)
    , m_port(port)
{
    registerRoutes();
}

void ApiServer::registerRoutes()
{
    using namespace std::placeholders;

    m_routes.push_back({ "GET",  "/health",             [this](auto& r){ return handleHealth(r);       } });
    m_routes.push_back({ "GET",  "/api/game/state",     [this](auto& r){ return handleGameState(r);    } });
    m_routes.push_back({ "POST", "/api/game/shot",      [this](auto& r){ return handleGameShot(r);     } });
    m_routes.push_back({ "POST", "/api/game/foul",      [this](auto& r){ return handleGameFoul(r);     } });
    m_routes.push_back({ "GET",  "/api/game/history",   [this](auto& r){ return handleGameHistory(r);  } });
    m_routes.push_back({ "POST", "/api/camera/",        [this](auto& r){ return handleCameraFrame(r);  } }); // /api/camera/{id}/frame
    m_routes.push_back({ "GET",  "/api/camera/fused",   [this](auto& r){ return handleCameraFused(r);  } });
    m_routes.push_back({ "GET",  "/api/simulate/demo",  [this](auto& r){ return handleSimulateDemo(r); } });
}

// -------------------------------------------------------------------
// Handlers des routes
// -------------------------------------------------------------------

HttpResponse ApiServer::handleHealth(const HttpRequest&)
{
    return { 200, "application/json",
             "{ \"status\": \"ok\", \"service\": \"snooker-player-3.0\" }" };
}

HttpResponse ApiServer::handleGameState(const HttpRequest&)
{
    return { 200, "application/json", gameStateToJson(m_manager) };
}

HttpResponse ApiServer::handleGameShot(const HttpRequest& req)
{
    try
    {
        Ball ball = parseShotRequest(req.body);
        Frame& frame = m_manager.getMatch().getCurrentFrame();
        bool ok = frame.playShot(ball);

        if (ok)
        {
            return { 200, "application/json",
                     successJson("Coup joue : " + ball.getName()) };
        }
        else
        {
            return { 200, "application/json",
                     errorJson("Faute enregistree") };
        }
    }
    catch (const std::exception& e)
    {
        return { 400, "application/json", errorJson(e.what()) };
    }
}

HttpResponse ApiServer::handleGameFoul(const HttpRequest& req)
{
    // Format: {"points":4}
    int points = 4;
    size_t pos = req.body.find("\"points\"");
    if (pos != std::string::npos)
    {
        pos = req.body.find(':', pos);
        if (pos != std::string::npos)
        {
            std::string num;
            ++pos;
            while (pos < req.body.size() &&
                   (req.body[pos] == ' ' || req.body[pos] == '\t')) ++pos;
            while (pos < req.body.size() && std::isdigit(req.body[pos]))
            {
                num += req.body[pos++];
            }
            if (!num.empty()) points = std::stoi(num);
        }
    }

    Frame& frame = m_manager.getMatch().getCurrentFrame();
    frame.foul(points);
    return { 200, "application/json",
             successJson("Faute declaree : " + std::to_string(points) + " points") };
}

HttpResponse ApiServer::handleGameHistory(const HttpRequest&)
{
    const Frame& frame = m_manager.getMatch().getCurrentFrame();
    return { 200, "application/json", shotHistoryToJson(frame.getHistory()) };
}

HttpResponse ApiServer::handleCameraFrame(const HttpRequest& req)
{
    // Extraire camera_id depuis l'URL: /api/camera/{id}/frame
    int cam_id = 1;
    size_t pos = req.path.find("/api/camera/");
    if (pos != std::string::npos)
    {
        pos += std::string("/api/camera/").size();
        std::string id_str;
        while (pos < req.path.size() && std::isdigit(req.path[pos]))
        {
            id_str += req.path[pos++];
        }
        if (!id_str.empty()) cam_id = std::stoi(id_str);
    }

    // Parse le JSON minimal (tableau de detections)
    // Format: {"timestamp_ms":1234,"detections":[{"color":"Noir","x_px":100,"y_px":200,"confidence":0.9}]}
    Camera::CameraFrame frame;
    frame.camera_id = cam_id;
    frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Parse timestamp si present
    pos = req.body.find("\"timestamp_ms\"");
    if (pos != std::string::npos)
    {
        pos = req.body.find(':', pos);
        if (pos != std::string::npos)
        {
            std::string num;
            ++pos;
            while (pos < req.body.size() && (req.body[pos] == ' ' || req.body[pos] == '\t')) ++pos;
            while (pos < req.body.size() && std::isdigit(req.body[pos])) num += req.body[pos++];
            if (!num.empty()) frame.timestamp_ms = std::stoll(num);
        }
    }

    // Parse les detections (positions en pixels, convertir en mm)
    auto& calibration = m_fusion.getCalibration(cam_id);
    pos = 0;
    while ((pos = req.body.find("\"color\"", pos)) != std::string::npos)
    {
        Camera::BallDetection det;
        det.camera_id    = cam_id;
        det.timestamp_ms = frame.timestamp_ms;
        det.confidence   = 0.9f;

        // color
        pos = req.body.find(':', pos);
        if (pos == std::string::npos) break;
        size_t start = req.body.find('"', pos + 1);
        if (start == std::string::npos) break;
        size_t end = req.body.find('"', start + 1);
        if (end == std::string::npos) break;
        det.color = req.body.substr(start + 1, end - start - 1);
        pos = end + 1;

        // x_px
        size_t xpos = req.body.find("\"x_px\"", pos);
        size_t ypos = req.body.find("\"y_px\"", pos);
        size_t cpos = req.body.find("\"confidence\"", pos);
        size_t next = req.body.find("\"color\"", pos);

        auto readFloat = [&](size_t fpos) -> float {
            if (fpos == std::string::npos) return 0.0f;
            fpos = req.body.find(':', fpos);
            if (fpos == std::string::npos) return 0.0f;
            ++fpos;
            while (fpos < req.body.size() && (req.body[fpos] == ' ' || req.body[fpos] == '\t')) ++fpos;
            std::string num;
            while (fpos < req.body.size() && (std::isdigit(req.body[fpos]) || req.body[fpos] == '.' || req.body[fpos] == '-'))
                num += req.body[fpos++];
            return num.empty() ? 0.0f : std::stof(num);
        };

        if (xpos != std::string::npos && (next == std::string::npos || xpos < next))
        {
            float x_px = readFloat(xpos);
            float y_px = readFloat(ypos);
            float x_mm, y_mm;
            if (calibration.pixelToTable(x_px, y_px, x_mm, y_mm))
            {
                det.x_mm = x_mm;
                det.y_mm = y_mm;
            }
        }
        if (cpos != std::string::npos && (next == std::string::npos || cpos < next))
        {
            det.confidence = readFloat(cpos);
        }

        frame.detections.push_back(det);
    }

    m_fusion.submitFrame(frame);

    return { 200, "application/json",
             successJson("Frame camera " + std::to_string(cam_id) +
                         " : " + std::to_string(frame.detections.size()) +
                         " bille(s) detectee(s)") };
}

HttpResponse ApiServer::handleCameraFused(const HttpRequest&)
{
    Camera::FusedGameState state = m_fusion.getFusedState();
    return { 200, "application/json", fusedStateToJson(state) };
}

HttpResponse ApiServer::handleSimulateDemo(const HttpRequest&)
{
    // Simuler quelques coups pour demo
    Frame& frame = m_manager.getMatch().getCurrentFrame();

    std::ostringstream oss;
    oss << "{\n  \"status\": \"ok\",\n  \"shots\": [\n";

    // Rouge + Noir
    bool s1 = frame.playShot(Ball("Rouge", 1));
    oss << "    { \"ball\": \"Rouge\", \"result\": \"" << (s1 ? "ok" : "foul") << "\" },\n";

    bool s2 = frame.playShot(Ball("Noir", 7));
    oss << "    { \"ball\": \"Noir\", \"result\": \"" << (s2 ? "ok" : "foul") << "\" }\n";

    oss << "  ],\n";
    oss << "  \"game_state\": " << gameStateToJson(m_manager) << "\n";
    oss << "}";
    return { 200, "application/json", oss.str() };
}

// -------------------------------------------------------------------
// Serveur TCP / HTTP
// -------------------------------------------------------------------

HttpRequest ApiServer::parseRequest(const std::string& raw)
{
    HttpRequest req;
    std::istringstream iss(raw);

    // Premiere ligne : METHOD PATH HTTP/1.x
    std::string line;
    std::getline(iss, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream first(line);
    first >> req.method >> req.path;

    // Headers (ignores sauf Content-Length)
    int content_length = 0;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break; // fin des headers

        if (line.find("Content-Length:") == 0 || line.find("content-length:") == 0)
        {
            size_t pos = line.find(':');
            if (pos != std::string::npos)
            {
                content_length = std::stoi(line.substr(pos + 1));
            }
        }
    }

    // Body
    if (content_length > 0)
    {
        std::string body(content_length, '\0');
        iss.read(&body[0], content_length);
        req.body = body;
    }

    return req;
}

std::string ApiServer::serializeResponse(const HttpResponse& resp)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << resp.status;
    switch (resp.status)
    {
    case 200: oss << " OK";          break;
    case 400: oss << " Bad Request"; break;
    case 404: oss << " Not Found";   break;
    default:  oss << " Error";       break;
    }
    oss << "\r\n";
    oss << "Content-Type: " << resp.content_type << "; charset=utf-8\r\n";
    oss << "Content-Length: " << resp.body.size() << "\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << resp.body;
    return oss.str();
}

HttpResponse ApiServer::handleRequest(const HttpRequest& req)
{
    // OPTIONS preflight CORS
    if (req.method == "OPTIONS")
    {
        return { 200, "text/plain", "" };
    }

    for (const auto& route : m_routes)
    {
        if (route.method != req.method) continue;

        // Match exact ou prefixe (pour /api/camera/{id}/...)
        if (req.path == route.path ||
            (!route.path.empty() && route.path.back() == '/' &&
             req.path.find(route.path) == 0))
        {
            return route.handler(req);
        }
    }

    return { 404, "application/json",
             errorJson("Route non trouvee : " + req.method + " " + req.path) };
}

void ApiServer::run()
{
    start();

    std::cout << "Serveur Snooker Player 3.0 en ecoute sur le port "
              << m_port << std::endl;
    std::cout << "  GET  http://localhost:" << m_port << "/health" << std::endl;
    std::cout << "  GET  http://localhost:" << m_port << "/api/game/state" << std::endl;
    std::cout << "  POST http://localhost:" << m_port << "/api/game/shot" << std::endl;
    std::cout << "  GET  http://localhost:" << m_port << "/api/simulate/demo" << std::endl;
    std::cout << "Appuyer sur Ctrl+C pour arreter." << std::endl;

    while (m_running)
    {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);

        int client_fd = accept(m_serverFd,
                               reinterpret_cast<sockaddr*>(&client_addr),
                               &client_len);
        if (client_fd < 0)
        {
            if (!m_running) break;
            continue;
        }

        // Lire la requete
        std::string raw;
        char buf[4096];
        ssize_t n;
        while ((n = recv(client_fd, buf, sizeof(buf) - 1, 0)) > 0)
        {
            buf[n] = '\0';
            raw += buf;
            // Fin des headers + body complet (heuristique simple)
            if (raw.find("\r\n\r\n") != std::string::npos)
            {
                // Verifier Content-Length
                size_t cl_pos = raw.find("Content-Length:");
                if (cl_pos == std::string::npos)
                    cl_pos = raw.find("content-length:");
                if (cl_pos != std::string::npos)
                {
                    size_t cl_end = raw.find("\r\n", cl_pos);
                    int content_length = std::stoi(raw.substr(cl_pos + 15, cl_end - cl_pos - 15));
                    size_t body_start = raw.find("\r\n\r\n") + 4;
                    if ((int)(raw.size() - body_start) >= content_length) break;
                }
                else
                {
                    break; // Pas de body
                }
            }
        }

        if (!raw.empty())
        {
            HttpRequest  req  = parseRequest(raw);
            HttpResponse resp = handleRequest(req);
            std::string  out  = serializeResponse(resp);
            send(client_fd, out.c_str(), static_cast<int>(out.size()), 0);
        }

#ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
    }
}

void ApiServer::start()
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    m_serverFd = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    if (m_serverFd < 0)
    {
        throw std::runtime_error("Impossible de creer le socket serveur");
    }

    int opt = 1;
    setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(m_port));

    if (bind(m_serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        throw std::runtime_error("bind() echoue sur le port " + std::to_string(m_port));
    }

    if (listen(m_serverFd, 10) < 0)
    {
        throw std::runtime_error("listen() echoue");
    }

    m_running = true;
}

void ApiServer::stop()
{
    m_running = false;
#ifdef _WIN32
    closesocket(m_serverFd);
    WSACleanup();
#else
    close(m_serverFd);
#endif
}

} // namespace Api
