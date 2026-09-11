#include "CueSenseLauncher.h"

#include "httplib.h"

#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QString>

namespace
{
    // Chemin local du dossier CueSense (fichiers statiques a plat, voir
    // son README). A ajuster ici si ce dossier est deplace/reorganise.
    const char* kCueSenseDir = "C:/CueSense/cuesense-main/cuesense-main";
}

CueSenseLauncher::CueSenseLauncher() = default;

CueSenseLauncher::~CueSenseLauncher()
{
    if (m_server)
    {
        m_server->stop();
    }
    if (m_serverThread.joinable())
    {
        m_serverThread.join();
    }
}

bool CueSenseLauncher::launch()
{
    if (!m_started)
    {
        if (!QDir(kCueSenseDir).exists())
        {
            return false;
        }

        m_server = std::make_unique<httplib::Server>();
        if (!m_server->set_mount_point("/", kCueSenseDir))
        {
            m_server.reset();
            return false;
        }

        bool bound = false;
        for (int attempt = 0; attempt < 10; ++attempt)
        {
            int candidatePort = 8090 + attempt;
            // 127.0.0.1 seulement (pas 0.0.0.0 comme MatchWebServer) :
            // CueSense n'a pas besoin d'etre joignable depuis un autre
            // appareil, c'est un lancement local pour le PC lui-meme.
            if (m_server->bind_to_port("127.0.0.1", candidatePort))
            {
                m_port = candidatePort;
                bound = true;
                break;
            }
        }
        if (!bound)
        {
            m_server.reset();
            return false;
        }

        m_serverThread = std::thread([this]()
            {
                m_server->listen_after_bind();
            });
        m_started = true;
    }

    QDesktopServices::openUrl(QUrl("http://127.0.0.1:" + QString::number(m_port) + "/index.html"));
    return true;
}
